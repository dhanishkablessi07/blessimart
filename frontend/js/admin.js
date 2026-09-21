// Admin panel: list users, list all orders, delete any product.
async function loadAdmin() {
  const u = await apiGet('/api/admin/users');
  const userTbody = document.getElementById('userRows');
  userTbody.innerHTML = '';
  if (!u.ok) { showMsg('msg', u.data.message || 'Admin login required', true); return; }
  u.data.users.forEach((x) => {
    const tr = document.createElement('tr');
    tr.innerHTML = `<td>${x.id}</td><td>${escapeHtml(x.name)}</td><td>${escapeHtml(x.email)}</td><td>${x.role}</td>`;
    userTbody.appendChild(tr);
  });

  const o = await apiGet('/api/admin/orders');
  const orderTbody = document.getElementById('orderRows');
  orderTbody.innerHTML = '';
  if (o.ok) {
    o.data.orders.forEach((x) => {
      const tr = document.createElement('tr');
      tr.innerHTML = `<td>#${x.id}</td><td>${escapeHtml(x.buyer_name)}</td><td>${x.total_price}</td><td>${x.status}</td>`;
      orderTbody.appendChild(tr);
    });
  }
}

document.getElementById('delBtn').addEventListener('click', async () => {
  const id = document.getElementById('delId').value;
  if (!id) return;
  if (!confirm('Delete product ' + id + '?')) return;
  const res = await fetch('/api/admin/products/' + id, { method: 'DELETE' });
  const data = await res.json().catch(() => ({}));
  showMsg('msg', data.message || (res.ok ? 'Deleted' : 'Failed'), !res.ok);
});

function escapeHtml(s) {
  return String(s || '').replace(/[&<>"']/g, (c) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  }[c]));
}

document.addEventListener('DOMContentLoaded', loadAdmin);
