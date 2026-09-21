// Cart page: list, change qty, remove. Total comes from server.
async function loadCart() {
  const { ok, data } = await apiGet('/api/cart');
  const tbody = document.getElementById('rows');
  tbody.innerHTML = '';
  if (!ok) {
    showMsg('msg', data.message || 'Buyer login required', true);
    return;
  }
  if (data.items.length === 0) {
    showMsg('msg', 'Cart is empty. Go shop!', false);
    document.getElementById('total').textContent = '';
    return;
  }
  data.items.forEach((it) => {
    const tr = document.createElement('tr');
    tr.innerHTML =
      `<td>${escapeHtml(it.title)}</td><td>${it.price}</td>` +
      `<td><input type="number" min="1" max="${it.stock}" value="${it.quantity}" data-qty="${it.id}" style="width:60px"></td>` +
      `<td>${it.line_total}</td>` +
      `<td><button data-update="${it.id}">Update</button> <button data-del="${it.id}">Remove</button></td>`;
    tbody.appendChild(tr);
  });
  document.getElementById('total').textContent = 'Total: Rs. ' + data.total;

  tbody.querySelectorAll('[data-update]').forEach((b) => {
    b.onclick = async () => {
      const id = b.dataset.update;
      const input = tbody.querySelector(`[data-qty="${id}"]`);
      const quantity = parseInt(input.value);
      const res = await fetch('/api/cart/' + id, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ quantity }),
      });
      const d = await res.json().catch(() => ({}));
      showMsg('msg', d.message || (res.ok ? 'Updated' : 'Failed'), !res.ok);
      loadCart();
    };
  });
  tbody.querySelectorAll('[data-del]').forEach((b) => {
    b.onclick = async () => {
      const res = await fetch('/api/cart/' + b.dataset.del, { method: 'DELETE' });
      const d = await res.json().catch(() => ({}));
      showMsg('msg', d.message || (res.ok ? 'Removed' : 'Failed'), !res.ok);
      loadCart();
    };
  });
}

function escapeHtml(s) {
  return String(s || '').replace(/[&<>"']/g, (c) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  }[c]));
}

document.addEventListener('DOMContentLoaded', loadCart);
