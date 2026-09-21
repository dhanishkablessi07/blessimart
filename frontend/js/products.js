// Home page: load + search/filter products.
async function loadProducts() {
  const search = document.getElementById('search').value.trim();
  const category = document.getElementById('category').value;
  const params = new URLSearchParams();
  if (search) params.set('search', search);
  if (category && category !== 'All') params.set('category', category);

  const { ok, data } = await apiGet('/api/products?' + params.toString());
  const grid = document.getElementById('grid');
  grid.innerHTML = '';
  if (!ok) {
    showMsg('msg', data.message || 'Failed to load products', true);
    return;
  }
  if (data.products.length === 0) {
    showMsg('msg', 'No products found.', true);
    return;
  }
  showMsg('msg', `${data.products.length} product(s)`, false);
  data.products.forEach((p) => {
    const div = document.createElement('div');
    div.className = 'card';
    div.innerHTML =
      `<h3><a href="product.html?id=${p.id}">${escapeHtml(p.title)}</a></h3>` +
      `<p>Rs. ${p.price} | Stock: ${p.stock}</p>` +
      `<p><small>${escapeHtml(p.category)} | by ${escapeHtml(p.seller_name || '')}</small></p>`;
    grid.appendChild(div);
  });
}

// Tiny helper to avoid breaking HTML with < >.
function escapeHtml(s) {
  return String(s || '').replace(/[&<>"']/g, (c) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  }[c]));
}

document.getElementById('searchBtn').addEventListener('click', loadProducts);
document.addEventListener('DOMContentLoaded', loadProducts);
