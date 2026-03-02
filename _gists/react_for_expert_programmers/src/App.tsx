// import { useState } from 'react'
// import reactLogo from './assets/react.svg'
//import viteLogo from '/vite.svg'
import { useState } from 'react';
import './App.css'
import ButtonWithSharedState from './ButtonWithSharedState';
// import BasicButtonEvent from './BasicEventHandler';
import ShoppingList from './ListOfItems';


const user = {
  name: "walcht",
  imageUrl: "https://i.imgur.com/yXOvdOSs.jpg",
  imageSize: 90,
};

function App() {

  /* create shared state here - count and clbk will be passed to child btns */
  const [count, setCount] = useState<number>(0);
  const clbk = () => { setCount(prevCount => prevCount + 1) };

  // ...

  return (
    /* This is an empty holder in react - you can't return multiple components.
      {} is the way to run JavaScript within React markup.
      You CANNOT write comments inside the <> brackets - it is markup and is
      interpreted as such */
    <>
      <h1>{user.name}</h1>
      <img
        className="avatar"
        /* you can obviously access data (as you would with vanilla JS) */
        src={user.imageUrl}
        alt={"photo of " + user.name} style={{
          width: user.imageSize,
          height: user.imageSize
        }} />
      <ShoppingList />
      <ButtonWithSharedState sharedCount={count} clbk={clbk} />
      <ButtonWithSharedState sharedCount={count} clbk={clbk} />
    </>
  )
}

export default App;
