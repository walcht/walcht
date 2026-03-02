const products = [
  { title: "idk_0", isAvailable: false, id: 0 },
  { title: "idk_1", isAvailable: true, id: 1 },
  { title: "idk_2", isAvailable: false, id: 2 },
];

function ShoppingList() {
  /* 'key' is a React attribute (not part of HTML DOM specs) that is used to
  * uniquely identify elements */
  const items = products.map((p) =>
    <li
      key={p.id}
      style={{
        color: p.isAvailable ? "green" : "red"
      }}
    >
      {p.title}
    </li>);
  /* you do not have to explicitly expand array elements in React - JSX.Element
  * arrays are automatically expanded */
  return <ul>{items}</ul>;
}

export default ShoppingList;
