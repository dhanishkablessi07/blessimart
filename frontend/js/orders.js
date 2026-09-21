// Buyer order history + status. Click an order to see items.
async function loadOrders() {
  const { ok, data } = await apiGet('/api/orders');
  const box = document.getElementById('list');
  box.innerHTML = '';
  if (!ok) { showMsg('msg', data.message || 'Buyer login required', true); return; }
  if (data.orders.length === 0) { showMsg('msg', 'No orders yet.', false); return; }
  data.orders.forEach((o) => {
    const div = document.createElement('div');
    div.className = 'card';
    div.innerHTML =
      `<b>Order #${o.id}</b> - Rs. ${o.total_price} - <span>[${escapeHtml(o.status)}]</span><br>` +
      `<small>${escapeHtml(o.created_at)} | ${escapeHtml(o.address)}</small><br>` +
      `<button data-view="${o.id}">View Items</button><div data-items="${o.id}"></div>`;
    box.appendChild(div);
  });
  box.querySelectorAll('[data-view]').forEach((b) => {
    b.onclick = async () => {
      const id = b.dataset.view;
      const target = box.querySelector(`[data-items="${id}"]`);
      const r = await apiGet('/api/orders/' + id);
      if (!r.ok) { target.textContent = r.data.message || 'Failed'; return; }
      target.innerHTML = '<ul>' + r.data.items.map((it) =>
        `<li>${escapeHtml(it.title)} x ${it.quantity} @ Rs. ${it.price_at_buy}</li>`).join('') + '</ul>';
    };
  });
}

function escapeHtml(s) {
  return String(s || '').replace(/[&<>"']/g, (c) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  }[c]));
}

document.addEventListener('DOMContentLoaded', loadOrders);
