import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';

void main() {
  runApp(const SecurityApp());
}

class SecurityApp extends StatelessWidget {
  const SecurityApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Smart Security',
      theme: ThemeData(
        brightness: Brightness.dark,
        colorSchemeSeed: Colors.teal,
        scaffoldBackgroundColor: const Color(0xff0f141a),
        useMaterial3: true,
      ),
      home: const DashboardPage(),
    );
  }
}

class DashboardPage extends StatefulWidget {
  const DashboardPage({super.key});

  @override
  State<DashboardPage> createState() => _DashboardPageState();
}

class _DashboardPageState extends State<DashboardPage> {
  final serverController =
      TextEditingController(text: 'http://192.168.0.101:5000');
  Timer? refreshTimer;
  Map<String, dynamic> status = {};
  List<dynamic> events = [];
  List<dynamic> images = [];
  String? error;
  bool loading = true;

  String get serverUrl => serverController.text.trim().replaceAll(RegExp(r'/$'), '');

  @override
  void initState() {
    super.initState();
    refresh();
    refreshTimer = Timer.periodic(const Duration(seconds: 4), (_) => refresh(silent: true));
  }

  @override
  void dispose() {
    refreshTimer?.cancel();
    serverController.dispose();
    super.dispose();
  }

  Future<Map<String, dynamic>> requestJson(String path, {String method = 'GET'}) async {
    final client = HttpClient();
    try {
      final uri = Uri.parse('$serverUrl$path');
      final request = method == 'POST'
          ? await client.postUrl(uri)
          : await client.getUrl(uri);
      request.headers.set(HttpHeaders.acceptHeader, 'application/json');
      final response = await request.close().timeout(const Duration(seconds: 8));
      final body = await utf8.decoder.bind(response).join();
      if (response.statusCode < 200 || response.statusCode >= 300) {
        throw HttpException('HTTP ${response.statusCode}: $body');
      }
      return jsonDecode(body) as Map<String, dynamic>;
    } finally {
      client.close(force: true);
    }
  }

  Future<void> refresh({bool silent = false}) async {
    if (!silent && mounted) {
      setState(() => loading = true);
    }
    try {
      final data = await requestJson('/api/dashboard?limit=30');
      if (!mounted) return;
      setState(() {
        status = data['status'] as Map<String, dynamic>? ?? {};
        events = data['events'] as List<dynamic>? ?? [];
        images = data['images'] as List<dynamic>? ?? [];
        error = null;
        loading = false;
      });
    } catch (exception) {
      if (!mounted) return;
      setState(() {
        error = exception.toString();
        loading = false;
      });
    }
  }

  Future<void> setArmed(bool armed) async {
    try {
      await requestJson(armed ? '/arm' : '/disarm', method: 'POST');
      await refresh();
    } catch (exception) {
      if (!mounted) return;
      setState(() => error = exception.toString());
    }
  }

  Widget statusCard(String title, String value, IconData icon, Color color) {
    return SizedBox(
      width: 170,
      child: Card(
        color: const Color(0xff17202a),
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Icon(icon, color: color),
              const SizedBox(height: 14),
              Text(title, style: const TextStyle(color: Colors.white60)),
              const SizedBox(height: 4),
              Text(value, style: const TextStyle(fontWeight: FontWeight.bold)),
            ],
          ),
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final armed = status['system_status'] == 'ARMED';
    return Scaffold(
      appBar: AppBar(
        title: const Text('Smart Security'),
        actions: [
          IconButton(onPressed: refresh, icon: const Icon(Icons.refresh)),
        ],
      ),
      body: RefreshIndicator(
        onRefresh: refresh,
        child: ListView(
          padding: const EdgeInsets.all(16),
          children: [
            TextField(
              controller: serverController,
              keyboardType: TextInputType.url,
              decoration: InputDecoration(
                labelText: 'Flask server URL',
                suffixIcon: IconButton(
                  icon: const Icon(Icons.check),
                  onPressed: refresh,
                ),
              ),
            ),
            const SizedBox(height: 16),
            if (loading) const LinearProgressIndicator(),
            if (error != null)
              Padding(
                padding: const EdgeInsets.only(bottom: 16),
                child: Text(error!, style: const TextStyle(color: Colors.redAccent)),
              ),
            Wrap(
              spacing: 12,
              runSpacing: 12,
              children: [
                statusCard(
                  'System',
                  status['system_status']?.toString() ?? '-',
                  armed ? Icons.security : Icons.shield_outlined,
                  armed ? Colors.redAccent : Colors.greenAccent,
                ),
                statusCard(
                  'PIR',
                  status['motion_status']?.toString() ?? '-',
                  Icons.directions_walk,
                  Colors.orangeAccent,
                ),
                statusCard(
                  'Door',
                  status['door_status']?.toString() ?? '-',
                  Icons.door_front_door,
                  Colors.blueAccent,
                ),
                statusCard(
                  'Camera',
                  status['camera_status']?.toString() ?? '-',
                  Icons.camera_alt,
                  Colors.purpleAccent,
                ),
              ],
            ),
            const SizedBox(height: 20),
            Row(
              children: [
                Expanded(
                  child: FilledButton.icon(
                    onPressed: () => setArmed(true),
                    icon: const Icon(Icons.lock),
                    label: const Text('ARM'),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: OutlinedButton.icon(
                    onPressed: () => setArmed(false),
                    icon: const Icon(Icons.lock_open),
                    label: const Text('DISARM'),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 24),
            const Text('Latest photos', style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold)),
            const SizedBox(height: 12),
            SizedBox(
              height: 180,
              child: images.isEmpty
                  ? const Card(child: Center(child: Text('No photos yet')))
                  : ListView.separated(
                      scrollDirection: Axis.horizontal,
                      itemCount: images.length,
                      separatorBuilder: (_, __) => const SizedBox(width: 12),
                      itemBuilder: (context, index) {
                        final image = images[index] as Map<String, dynamic>;
                        return ClipRRect(
                          borderRadius: BorderRadius.circular(16),
                          child: Image.network(
                            image['url'].toString(),
                            width: 240,
                            fit: BoxFit.cover,
                            errorBuilder: (_, __, ___) => const SizedBox(
                              width: 240,
                              child: Card(child: Center(child: Icon(Icons.broken_image))),
                            ),
                          ),
                        );
                      },
                    ),
            ),
            const SizedBox(height: 24),
            const Text('Event history', style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold)),
            const SizedBox(height: 12),
            ...events.map((item) {
              final event = item as Map<String, dynamic>;
              return Card(
                margin: const EdgeInsets.only(bottom: 10),
                child: ListTile(
                  leading: event['image_url'] == null
                      ? const Icon(Icons.notifications)
                      : ClipRRect(
                          borderRadius: BorderRadius.circular(8),
                          child: Image.network(
                            event['image_url'].toString(),
                            width: 52,
                            height: 52,
                            fit: BoxFit.cover,
                          ),
                        ),
                  title: Text(event['event_type']?.toString() ?? 'event'),
                  subtitle: Text(
                    '${event['occurred_at'] ?? ''}\n${event['description'] ?? ''}',
                  ),
                  isThreeLine: true,
                ),
              );
            }),
          ],
        ),
      ),
    );
  }
}
