// Send form data to the local server, which calls the C backend.
async function refresh() {
  try {
    const response = await fetch('/api/list');
    document.getElementById('events').textContent = await response.text();
  } catch (error) {
    document.getElementById('events').textContent = 'Could not reach the local server.';
  }
}

document.querySelectorAll('form').forEach(form => {
  form.addEventListener('submit', async event => {
    event.preventDefault();
    try {
      const response = await fetch('/api/' + form.dataset.action, {
        method: 'POST', body: new URLSearchParams(new FormData(form))
      });
      const message = await response.text();
      document.getElementById('message').textContent = message;
      if (response.ok) form.reset();
      await refresh();
    } catch (error) {
      document.getElementById('message').textContent = 'Could not reach the local server.';
    }
  });
});
document.getElementById('refresh').addEventListener('click', refresh);
refresh();
