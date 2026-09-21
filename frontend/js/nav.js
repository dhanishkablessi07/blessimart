// Shows login state in the top nav on every page.
// Expects: <span id="userInfo"> and <a id="logoutLink"> in the page.
async function loadNav() {
  const info = document.getElementById('userInfo');
  const logoutLink = document.getElementById('logoutLink');
  try {
    const { ok, data } = await apiGet('/api/me');
    if (ok) {
      if (info) info.textContent = `Hi, ${data.name} (${data.role})`;
      if (logoutLink) logoutLink.style.display = 'inline';
    } else {
      if (info) info.textContent = 'Not logged in';
      if (logoutLink) logoutLink.style.display = 'none';
    }
  } catch (e) {
    if (info) info.textContent = 'Server offline?';
  }
  if (logoutLink) {
    logoutLink.onclick = async (ev) => {
      ev.preventDefault();
      await apiPost('/api/logout', {});
      window.location.href = 'index.html';
    };
  }
}
document.addEventListener('DOMContentLoaded', loadNav);
