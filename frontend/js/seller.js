// Seller dashboard: list my products, add/edit/delete.
async function loadMine() {
  const res = await fetch('/api/seller/products');
  const data = await res.json().catch(() => ({}));
  const tbody = document.getElementById('rows');
  tbody.innerHTML = '';
  if (!res.ok) {
    showMsg('msg', data.message || 'Seller login required', true);
    return;
  }
  data.products.forEach((p) => {
    const tr = document.createElement('tr');
    tr.innerHTML =
      `<td>${p.id}</td><td>${escapeHtml(p.title)}</td><td>${p.price}</td>` +
      `<td>${p.stock}</td><td>${escapeHtml(p.category)}</td>` +
      `<td><button data-edit="${p.id}">Edit</button> <button data-del="${p.id}">Delete</button></td>`;
    tbody.appendChild(tr);
  });
  tbody.querySelectorAll('[data-edit]').forEach((b) => {
    b.onclick = () => startEdit(parseInt(b.dataset.edit), data.products);
  });
  tbody.querySelectorAll('[data-del]').forEach((b) => {
    b.onclick = () => delProduct(parseInt(b.dataset.del));
  });
}

function startEdit(id, products) {
  const p = products.find((x) => x.id === id);
  if (!p) return;
  document.getElementById('editId').value = p.id;
  document.getElementById('title').value = p.title;
  document.getElementById('description').value = p.description || '';
  document.getElementById('price').value = p.price;
  document.getElementById('stock').value = p.stock;
  document.getElementById('category').value = p.category;
  document.getElementById('saveBtn').textContent = 'Update Product';
  document.getElementById('cancelBtn').style.display = 'inline';
}

function resetForm() {
  document.getElementById('productForm').reset();
  document.getElementById('editId').value = '';
  document.getElementById('saveBtn').textContent = 'Add Product';
  document.getElementById('cancelBtn').style.display = 'none';
}

async function delProduct(id) {
  if (!confirm('Delete product ' + id + '?')) return;
  const res = await fetch('/api/seller/products/' + id, { method: 'DELETE' });
  const data = await res.json().catch(() => ({}));
  showMsg('msg', data.message || (res.ok ? 'Deleted' : 'Failed'), !res.ok);
  loadMine();
}

document.getElementById('productForm').addEventListener('submit', async (e) => {
  e.preventDefault();
  const body = {
    title: document.getElementById('title').value.trim(),
    description: document.getElementById('description').value.trim(),
    price: parseFloat(document.getElementById('price').value),
    stock: parseInt(document.getElementById('stock').value),
    category: document.getElementById('category').value,
    image_url: '',
  };
  const editId = document.getElementById('editId').value;
  const url = editId ? '/api/seller/products/' + editId : '/api/seller/products';
  const method = editId ? 'PUT' : 'POST';
  const res = await fetch(url, {
    method, headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body),
  });
  const data = await res.json().catch(() => ({}));
  showMsg('msg', data.message || (res.ok ? 'Saved' : 'Failed'), !res.ok);
  if (res.ok) { resetForm(); loadMine(); }
});

document.getElementById('cancelBtn').addEventListener('click', resetForm);

function escapeHtml(s) {
  return String(s || '').replace(/[&<>"']/g, (c) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  }[c]));
}

// Received orders: flat sales list + status update.
async function loadSales() {
  const res = await fetch('/api/seller/orders');
  const data = await res.json().catch(() => ({}));
  const tbody = document.getElementById('salesRows');
  if (!tbody) return;
  tbody.innerHTML = '';
  if (!res.ok) {
    showMsg('salesMsg', data.message || 'Seller login required', true);
    return;
  }
  if (data.sales.length === 0) {
    showMsg('salesMsg', 'No sales yet.', false);
    return;
  }
  data.sales.forEach((s) => {
    const tr = document.createElement('tr');
    tr.innerHTML =
      `<td>#${s.order_id}<br><small>${escapeHtml(s.created_at)}</small></td>` +
      `<td>${escapeHtml(s.buyer_name)}<br><small>${escapeHtml(s.address)}</small></td>` +
      `<td>${escapeHtml(s.title)} x ${s.quantity} @ ${s.price_at_buy}</td>` +
      `<td>${escapeHtml(s.status)}</td>` +
      `<td><select data-status="${s.order_id}">` +
      ['placed', 'shipped', 'delivered', 'cancelled'].map((st) =>
        `<option value="${st}"${st === s.status ? ' selected' : ''}>${st}</option>`).join('') +
      `</select> <button data-save="${s.order_id}">Save</button></td>`;
    tbody.appendChild(tr);
  });
  tbody.querySelectorAll('[data-save]').forEach((b) => {
    b.onclick = async () => {
      const orderId = b.dataset.save;
      const sel = tbody.querySelector(`[data-status="${orderId}"]`);
      const res2 = await fetch('/api/seller/orders/' + orderId, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ status: sel.value }),
      });
      const d2 = await res2.json().catch(() => ({}));
      showMsg('salesMsg', d2.message || (res2.ok ? 'Updated' : 'Failed'), !res2.ok);
      loadSales();
    };
  });
}

document.addEventListener('DOMContentLoaded', () => { loadMine(); loadSales(); });
