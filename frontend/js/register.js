// Register page logic: POST /api/register, then go to login.
document.getElementById('registerForm').addEventListener('submit', async (e) => {
  e.preventDefault();
  const name = document.getElementById('name').value.trim();
  const email = document.getElementById('email').value.trim();
  const password = document.getElementById('password').value;
  const role = document.getElementById('role').value;
  const { ok, data } = await apiPost('/api/register', { name, email, password, role });
  if (ok) {
    showMsg('msg', 'Registered! Going to login...', false);
    setTimeout(() => { window.location.href = 'login.html'; }, 800);
  } else {
    showMsg('msg', data.message || 'Register failed', true);
  }
});
