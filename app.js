// CampusConnect - Frontend Client Logic

// DOM Elements
const eventsGrid = document.getElementById('events-grid');
const historyContainer = document.getElementById('history-container');
const rawEventsPre = document.getElementById('raw-events');
const rawViewWrapper = document.getElementById('raw-view-wrapper');
const toggleRawBtn = document.getElementById('toggle-raw-btn');
const messageBox = document.getElementById('message');
const searchInput = document.getElementById('search-input');
const searchBtn = document.getElementById('search-btn');
const clearSearchBtn = document.getElementById('clear-search-btn');
const searchStatus = document.getElementById('search-status');
const refreshBtn = document.getElementById('refresh-btn');
const refreshHistoryBtn = document.getElementById('refresh-history-btn');

// Stats Elements
const statTotalEvents = document.getElementById('stat-total-events');
const statTotalConfirmed = document.getElementById('stat-total-confirmed');
const statTotalWaiting = document.getElementById('stat-total-waiting');

// Utility: Show User Notification
function showMessage(text, isSuccess = true) {
  messageBox.textContent = text;
  messageBox.className = isSuccess ? 'success' : 'error';
  messageBox.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
}

function clearMessage() {
  messageBox.textContent = '';
  messageBox.className = 'hidden';
}

// Parser: Parse C backend monospace output into structured Event objects
function parseEventsText(text) {
  const events = [];
  const eventBlocks = text.split(/\n(?=Event \d+:)/);

  for (const block of eventBlocks) {
    const trimmed = block.trim();
    if (!trimmed.startsWith('Event ')) continue;

    // Header: Event 1: Tech Symposium (Technical)
    const headerMatch = trimmed.match(/^Event\s+(\d+):\s+(.+?)\s+\((.+?)\)/m);
    if (!headerMatch) continue;

    const id = parseInt(headerMatch[1], 10);
    const name = headerMatch[2];
    const category = headerMatch[3];

    // Capacity: Capacity: 2 | Registered: 2 | Available: 0
    const capMatch = trimmed.match(/Capacity:\s+(\d+)\s+\|\s+Registered:\s+(\d+)\s+\|\s+Available:\s+(-?\d+)/m);
    const capacity = capMatch ? parseInt(capMatch[1], 10) : 0;
    const registeredCount = capMatch ? parseInt(capMatch[2], 10) : 0;
    const available = capMatch ? parseInt(capMatch[3], 10) : 0;

    // Confirmed Participants List (Singly Linked List)
    const confirmed = [];
    const confirmedSection = trimmed.match(/Confirmed participants:\n([\s\S]*?)(?=Waiting list|$)/);
    if (confirmedSection) {
      const lines = confirmedSection[1].split('\n');
      for (const line of lines) {
        const parts = line.trim().split('|').map(s => s.trim());
        if (parts.length === 3 && parts[0] !== 'None') {
          confirmed.push({ id: parts[0], name: parts[1], department: parts[2] });
        }
      }
    }

    // Waiting Queue (FIFO Queue)
    const waiting = [];
    const waitingSection = trimmed.match(/Waiting list \(first to last\):\n([\s\S]*?)(?=---|Found|\n\n|$)/);
    if (waitingSection) {
      const lines = waitingSection[1].split('\n');
      for (const line of lines) {
        const parts = line.trim().split('|').map(s => s.trim());
        if (parts.length === 3 && parts[0] !== 'None') {
          waiting.push({ id: parts[0], name: parts[1], department: parts[2] });
        }
      }
    }

    events.push({ id, name, category, capacity, registeredCount, available, confirmed, waiting });
  }

  return events;
}

// Parser: Parse C backend Stack history output
function parseHistoryText(text) {
  const items = [];
  const lines = text.split('\n');
  for (const line of lines) {
    const match = line.trim().match(/^\[(\d+)\]\s+(.+)$/);
    if (match) {
      items.push({ index: match[1], desc: match[2] });
    }
  }
  return items;
}

// Render Event Cards
function renderEventCards(events) {
  eventsGrid.innerHTML = '';

  if (!events || events.length === 0) {
    eventsGrid.innerHTML = `
      <div class="empty-state">
        <p><strong>No events found.</strong></p>
        <p>Use the "Add New Event" form above to create college events.</p>
      </div>`;
    updateStats(0, 0, 0);
    return;
  }

  let totalConfirmed = 0;
  let totalWaiting = 0;

  events.forEach(e => {
    totalConfirmed += e.confirmed.length;
    totalWaiting += e.waiting.length;

    const isFull = e.registeredCount >= e.capacity;
    const percentFilled = Math.min(100, Math.round((e.registeredCount / e.capacity) * 100));
    const catClass = e.category.toLowerCase().includes('tech') ? 'category-technical' :
                     e.category.toLowerCase().includes('cult') ? 'category-cultural' : 'category-other';

    const card = document.createElement('div');
    card.className = 'event-card';

    // Build Confirmed List HTML
    let confirmedHtml = '<li class="roster-empty">No confirmed participants</li>';
    if (e.confirmed.length > 0) {
      confirmedHtml = e.confirmed.map(p => `
        <li class="roster-item">
          <div><span class="roster-id">${escapeHtml(p.id)}</span> ${escapeHtml(p.name)}</div>
          <span class="text-muted">${escapeHtml(p.department)}</span>
        </li>
      `).join('');
    }

    // Build Waiting Queue HTML
    let waitingHtml = '<li class="roster-empty">Queue is empty</li>';
    if (e.waiting.length > 0) {
      waitingHtml = e.waiting.map((p, idx) => `
        <li class="roster-item">
          <div>
            <span class="queue-tag">#${idx + 1}</span>
            <span class="roster-id">${escapeHtml(p.id)}</span> ${escapeHtml(p.name)}
          </div>
          <span class="text-muted">${escapeHtml(p.department)}</span>
        </li>
      `).join('');
    }

    card.innerHTML = `
      <div class="event-card-top">
        <div>
          <span class="event-id-badge">Event ID: ${e.id}</span>
          <h3 class="event-title" style="margin-top: 4px;">${escapeHtml(e.name)}</h3>
        </div>
        <span class="category-badge ${catClass}">${escapeHtml(e.category)}</span>
      </div>

      <div class="event-meta">
        <span class="status-badge ${isFull ? 'status-full' : 'status-available'}">
          ${isFull ? 'CAPACITY FULL (Waitlist Active)' : 'SEATS AVAILABLE'}
        </span>
      </div>

      <div class="capacity-container">
        <div class="capacity-labels">
          <span>Capacity: <strong>${e.registeredCount} / ${e.capacity}</strong></span>
          <span>${e.available > 0 ? e.available + ' seats left' : '0 available'}</span>
        </div>
        <div class="capacity-bar">
          <div class="capacity-fill ${isFull ? 'full' : ''}" style="width: ${percentFilled}%"></div>
        </div>
      </div>

      <div class="roster-group">
        <div class="roster-title">
          <span>Confirmed Participants (${e.confirmed.length})</span>
          <span style="font-weight: normal; font-size: 11px;">Singly Linked List</span>
        </div>
        <ul class="roster-list">${confirmedHtml}</ul>
      </div>

      <div class="roster-group">
        <div class="roster-title">
          <span>Waiting List (${e.waiting.length})</span>
          <span style="font-weight: normal; font-size: 11px;">FIFO Queue</span>
        </div>
        <ul class="roster-list">${waitingHtml}</ul>
      </div>

      <div class="event-card-actions">
        <button class="secondary select-event-btn" data-event-id="${e.id}" type="button">
          Select for Registration &rarr;
        </button>
      </div>
    `;

    eventsGrid.appendChild(card);
  });

  updateStats(events.length, totalConfirmed, totalWaiting);

  // Attach quick-select handlers
  document.querySelectorAll('.select-event-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const eventId = btn.dataset.eventId;
      document.getElementById('reg-event-id').value = eventId;
      document.getElementById('cancel-event-id').value = eventId;
      document.getElementById('form-register').scrollIntoView({ behavior: 'smooth', block: 'center' });
      document.getElementById('reg-event-id').focus();
    });
  });
}

function updateStats(eventsCount, confirmedCount, waitingCount) {
  statTotalEvents.textContent = eventsCount;
  statTotalConfirmed.textContent = confirmedCount;
  statTotalWaiting.textContent = waitingCount;
}

// Render Stack History
function renderHistory(text) {
  const items = parseHistoryText(text);
  historyContainer.innerHTML = '';

  if (!items || items.length === 0) {
    historyContainer.innerHTML = '<p class="roster-empty" style="text-align: center; padding: 16px;">Stack is currently empty.</p>';
    return;
  }

  items.forEach(item => {
    const div = document.createElement('div');
    div.className = 'history-item';

    let tagClass = 'tag-reg';
    let label = 'ACTION';
    const lower = item.desc.toLowerCase();
    if (lower.includes('added event')) { tagClass = 'tag-added'; label = 'EVENT'; }
    else if (lower.includes('promoted')) { tagClass = 'tag-promoted'; label = 'PROMOTED'; }
    else if (lower.includes('waitlist')) { tagClass = 'tag-wait'; label = 'WAITLIST'; }
    else if (lower.includes('cancel')) { tagClass = 'tag-cancel'; label = 'CANCEL'; }
    else if (lower.includes('registered')) { tagClass = 'tag-reg'; label = 'CONFIRMED'; }

    div.innerHTML = `
      <span class="history-index">[${item.index}]</span>
      <span class="history-tag ${tagClass}">${label}</span>
      <span class="history-desc">${escapeHtml(item.desc)}</span>
    `;
    historyContainer.appendChild(div);
  });
}

// Escape HTML utility to prevent XSS
function escapeHtml(str) {
  if (!str) return '';
  return str.replace(/[&<>"']/g, m => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'
  })[m]);
}

// Load Events from /api/list
async function loadEvents() {
  try {
    const response = await fetch('/api/list');
    const text = await response.text();
    rawEventsPre.textContent = text;
    const events = parseEventsText(text);
    renderEventCards(events);
    searchStatus.textContent = '';
  } catch (error) {
    eventsGrid.innerHTML = '<p class="error-state">Could not connect to the local Python server.</p>';
    rawEventsPre.textContent = 'Server connection failed.';
  }
}

// Load Activity History from /api/history
async function loadHistory() {
  try {
    const response = await fetch('/api/history');
    const text = await response.text();
    renderHistory(text);
  } catch (error) {
    historyContainer.innerHTML = '<p class="roster-empty">Could not load activity history.</p>';
  }
}

// Search Events via /api/search
async function performSearch() {
  const query = searchInput.value.trim();
  if (!query) {
    loadEvents();
    return;
  }

  try {
    searchStatus.textContent = `Searching for "${query}"...`;
    const response = await fetch(`/api/search?q=${encodeURIComponent(query)}`);
    const text = await response.text();
    rawEventsPre.textContent = text;

    if (!response.ok) {
      showMessage(text, false);
      searchStatus.textContent = `Error: ${text}`;
      return;
    }

    const events = parseEventsText(text);
    renderEventCards(events);
    searchStatus.textContent = `Showing ${events.length} search result(s) for "${query}".`;
  } catch (error) {
    showMessage('Could not execute search request.', false);
  }
}

// Form Handlers
document.querySelectorAll('form').forEach(form => {
  form.addEventListener('submit', async event => {
    event.preventDefault();
    clearMessage();

    const action = form.dataset.action;
    try {
      const response = await fetch('/api/' + action, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams(new FormData(form))
      });

      const message = await response.text();
      showMessage(message, response.ok);

      if (response.ok) {
        form.reset();
      }

      await loadEvents();
      await loadHistory();
    } catch (error) {
      showMessage('Could not reach the local server.', false);
    }
  });
});

// Search & Filter Listeners
searchBtn.addEventListener('click', performSearch);
searchInput.addEventListener('keydown', e => {
  if (e.key === 'Enter') {
    e.preventDefault();
    performSearch();
  }
});

clearSearchBtn.addEventListener('click', () => {
  searchInput.value = '';
  searchStatus.textContent = '';
  loadEvents();
});

// Refresh Buttons
refreshBtn.addEventListener('click', () => {
  loadEvents();
  loadHistory();
});

refreshHistoryBtn.addEventListener('click', loadHistory);

// Terminal Raw View Toggle
toggleRawBtn.addEventListener('click', () => {
  const isHidden = rawViewWrapper.classList.toggle('hidden');
  toggleRawBtn.textContent = isHidden ? 'Terminal View' : 'Hide Terminal';
});

// Initial Page Load
loadEvents();
loadHistory();
