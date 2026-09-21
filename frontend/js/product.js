// Product detail page: GET /api/products?id from URL (?id=).
document.addEventListener('DOMContentLoaded', async () => {
  const id = new URLSearchParams(window.location.search).get('id');
  const box = document.getElementById('detail');
  if (!id) { box.innerHTML = '<p>Missing product id.</p>'; return; }
  const { ok, data } = await apiGet('/api/products/' + id);
  if (!ok) { box.innerHTML = `<p>${data.message || 'Not found'}</p>`; return; }
  box.innerHTML =
    `<h1>${escapeHtml(data.title)}</h1>` +
    `<p>Rs. ${data.price} | Stock: ${data.stock}</p>` +
    `<p><small>${escapeHtml(data.category)} | by ${escapeHtml(data.seller_name || '')}</small></p>` +
    `<p>${escapeHtml(data.description)}</p>` +
    `<label>Qty: <input type="number" id="qty" value="1" min="1" max="${data.stock}" style="width:60px"></label> ` +
    `<button id="addBtn">Add to Cart</button><p id="msg"></p>` +
    `<p><a href="index.html">Back to products</a> | <a href="cart.html">Go to Cart</a></p>`;
  document.getElementById('addBtn').onclick = async () => {
    const quantity = parseInt(document.getElementById('qty').value) || 1;
    const r = await apiPost('/api/cart', { product_id: parseInt(id), quantity });
    showMsg('msg', r.data.message || (r.ok ? 'Added!' : 'Failed'), !r.ok);
  };
  loadReviews(id);

  document.getElementById('reviewForm').addEventListener('submit', async (e) => {
    e.preventDefault();
    const rating = parseInt(document.getElementById('rating').value);
    const comment = document.getElementById('comment').value.trim();
    const r = await apiPost(`/api/products/${id}/reviews`, { rating, comment });
    showMsg('reviewMsg', r.data.message || (r.ok ? 'Added!' : 'Failed'), !r.ok);
    if (r.ok) { document.getElementById('comment').value = ''; loadReviews(id); }
  });
});

async function loadReviews(id) {
  const { ok, data } = await apiGet(`/api/products/${id}/reviews`);
  if (!ok) return;
  document.getElementById('avg').textContent =
    data.count === 0 ? 'No ratings yet.' : `Average: ${Number(data.avg).toFixed(1)} / 5 (${data.count} review(s))`;
  const box = document.getElementById('reviews');
  box.innerHTML = '';
  data.reviews.forEach((r) => {
    const div = document.createElement('div');
    div.className = 'card';
    div.innerHTML = `<b>${r.rating}/5</b> by ${escapeHtml(r.buyer_name)}<br>${escapeHtml(r.comment)}`;
    box.appendChild(div);
  });
}

function escapeHtml(s) {
  return String(s || '').replace(/[&<>"']/g, (c) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  }[c]));
}
