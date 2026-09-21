// Checkout: show cart summary, POST /api/checkout.
async function loadSummary() {
  const { ok, data } = await apiGet('/api/cart');
  const box = document.getElementById('summary');
  if (!ok) { box.textContent = data.message || 'Buyer login required'; return; }
  if (data.items.length === 0) { box.textContent = 'Cart is empty.'; return; }
  box.innerHTML =
    '<ul>' + data.items.map((it) =>
      `<li>${escapeHtml(it.title)} x ${it.quantity} = Rs. ${it.line_total}</li>`).join('') +
    `</ul><b>Total: Rs. ${data.total}</b>`;
}

document.getElementById('checkoutForm').addEventListener('submit', async (e) => {
  e.preventDefault();
  const address = document.getElementById('address').value.trim();
  const { ok, data } = await apiPost('/api/checkout', { address });
  if (ok) {
    showMsg('msg', `Order #${data.order_id} placed! Going to orders...`, false);
    setTimeout(() => { window.location.href = 'orders.html'; }, 800);
  } else {
    showMsg('msg', data.message || 'Checkout failed', true);
  }
});

function escapeHtml(s) {
  return String(s || '').replace(/[&<>"']/g, (c) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  }[c]));
}

document.addEventListener('DOMContentLoaded', loadSummary);
