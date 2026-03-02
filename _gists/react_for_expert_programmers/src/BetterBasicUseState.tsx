import { useState } from "react";

const sleep = (ms: number) => new Promise((r) => setTimeout(r, ms));

async function SomeVeryFuckingExpensiveComputation() {
  await sleep(2000);
  return Math.random() * 1000;
}

async function BetterBasicUseState() {
  /* log - so that we know when this is getting called */
  console.log("BetterBasicUseState has been called!");

  /*  */
  const [count, setCount] = useState<number>(await SomeVeryFuckingExpensiveComputation());

  /* this is the correct way to call setCount */
  return (
    <>
      <button onClick={() => {
        setCount((prevCount) => prevCount + 1);
        setCount((prevCount) => prevCount + 1);
      }
      }>
        increment two times ({count})
      </button>
    </>
  );
}

export default BetterBasicUseState;
