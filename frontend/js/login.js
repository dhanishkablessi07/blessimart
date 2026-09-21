// Login page logic: POST /api/login, then go to home.
document.getElementById('loginForm').addEventListener('submit', async (e) => {
  e.preventDefault();
  const email = document.getElementById('email').value.trim();
  const password = document.getElementById('password').value;
  const { ok, data } = await apiPost('/api/login', { email, password });
  if (ok) {
    showMsg('msg', `Welcome ${data.name}! Going home...`, false);
    setTimeout(() => { window.location.href = 'index.html'; }, 600);
  } else {
    showMsg('msg', data.message || 'Login failed', true);
  }
});
