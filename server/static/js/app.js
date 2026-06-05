const elements = {
  systemStatus: document.getElementById('system-status'),
  motionStatus: document.getElementById('motion-status'),
  doorStatus: document.getElementById('door-status'),
  cameraStatus: document.getElementById('camera-status'),
  motionUpdated: document.getElementById('motion-updated'),
  doorUpdated: document.getElementById('door-updated'),
  espEvent: document.getElementById('esp-event'),
  stateBadge: document.getElementById('state-badge'),
  eventTime: document.getElementById('event-time'),
  eventType: document.getElementById('event-type'),
  eventDescription: document.getElementById('event-description'),
  historyTable: document.getElementById('history-table-body'),
  galleryGrid: document.getElementById('gallery-grid'),
  armButton: document.getElementById('arm-button'),
  disarmButton: document.getElementById('disarm-button'),
};

async function fetchJson(path) {
  const response = await fetch(path);
  if (!response.ok) {
    throw new Error(`Failed to fetch ${path}`);
  }
  return response.json();
}

function inferStatusFromEvent(event, status) {
  if (!event || !event.event_type) {
    return status;
  }

  const type = event.event_type.toLowerCase();
  const nextStatus = { ...status };

  if (type.includes('door')) {
    if (type.includes('close') || type.includes('closed')) {
      nextStatus.door_status = 'Door Closed';
    } else if (type.includes('open') || type.includes('opened')) {
      nextStatus.door_status = 'Door Open';
    }
  }

  if (type.includes('motion')) {
    if (type.includes('end') || type.includes('no motion') || type.includes('stopped')) {
      nextStatus.motion_status = 'No Motion';
    } else {
      nextStatus.motion_status = 'Motion Detected';
    }
  }

  return nextStatus;
}

function updateStatus(status) {
  if (!elements.systemStatus) return;
  let displayStatus = { ...status };
  const event = status.last_event;
  if (event) {
    displayStatus = inferStatusFromEvent(event, displayStatus);
  }

  elements.systemStatus.textContent = displayStatus.system_status;
  elements.motionStatus.textContent = displayStatus.motion_status;
  elements.doorStatus.textContent = displayStatus.door_status;
  elements.cameraStatus.textContent = displayStatus.camera_status;
  if (elements.espEvent) {
    elements.espEvent.textContent = event ? `${event.event_type}: ${event.description}` : 'No event yet';
  }

  if (elements.motionUpdated) {
    if (event && event.event_type && event.event_type.includes('motion')) {
      elements.motionUpdated.textContent = `${event.occurred_at.replace('T', ' ')} - ${event.description}`;
    }
  }
  if (elements.doorUpdated) {
    const event = status.last_event;
    if (event && event.event_type && event.event_type.includes('door')) {
      elements.doorUpdated.textContent = `${event.occurred_at.replace('T', ' ')} - ${event.description}`;
    }
  }
  if (elements.stateBadge) {
    elements.stateBadge.textContent = status.system_status;
    elements.stateBadge.className = `state-badge ${status.system_status.toLowerCase()}`;
  }
  if (elements.eventTime) {
    elements.eventTime.textContent = status.last_event?.occurred_at?.replace('T', ' ') || 'No event yet';
  }
  if (elements.eventType) {
    elements.eventType.textContent = status.last_event?.event_type || 'No event yet';
  }
  if (elements.eventDescription) {
    elements.eventDescription.textContent = status.last_event?.description || 'No event yet';
  }
}

function renderEvents(events) {
  if (!elements.historyTable) return;
  elements.historyTable.innerHTML = events
    .map(event => `
      <tr>
        <td>${event.occurred_at.replace('T', ' ')}</td>
        <td>${event.event_type}</td>
        <td>${event.description}</td>
        <td>${event.image_url ? `<a class="image-link" href="${event.image_url}" target="_blank">View photo</a>` : '-'}</td>
      </tr>
    `)
    .join('');
}

function renderImages(images) {
  if (!elements.galleryGrid) return;
  if (!images.length) {
    elements.galleryGrid.innerHTML = '<p class="log-empty">No images available yet.</p>';
    return;
  }
  elements.galleryGrid.innerHTML = images
    .map(image => `
      <div class="gallery-card">
        <a href="${image.url}" target="_blank">
          <img src="${image.url}" alt="${image.filename}" loading="lazy" />
        </a>
        <div class="gallery-meta">
          <strong>${image.filename}</strong>
          <span>${image.modified}</span>
        </div>
      </div>
    `)
    .join('');
}

async function refreshDashboard() {
  try {
    const data = await fetchJson('/status');
    updateStatus(data);
  } catch (error) {
    console.warn(error);
  }
}

async function refreshHistory() {
  try {
    const { events } = await fetchJson('/events');
    renderEvents(events);
  } catch (error) {
    console.warn(error);
  }
}

async function refreshGallery() {
  try {
    const { alerts } = await fetchJson('/alerts');
    renderImages(alerts);
  } catch (error) {
    console.warn(error);
  }
}

async function sendAction(path) {
  try {
    const response = await fetch(path, { method: 'POST' });
    if (!response.ok) {
      throw new Error(`Request failed: ${path}`);
    }
    const state = await response.json();
    updateStatus(state);
    await refreshHistory();
  } catch (error) {
    console.warn(error);
  }
}

if (elements.armButton) {
  elements.armButton.addEventListener('click', () => sendAction('/arm'));
}

if (elements.disarmButton) {
  elements.disarmButton.addEventListener('click', () => sendAction('/disarm'));
}

if (elements.systemStatus) {
  refreshDashboard();
  setInterval(refreshDashboard, 3000);
}

if (elements.historyTable) {
  refreshHistory();
  setInterval(refreshHistory, 5000);
}

if (elements.galleryGrid) {
  refreshGallery();
  setInterval(refreshGallery, 7000);
}
