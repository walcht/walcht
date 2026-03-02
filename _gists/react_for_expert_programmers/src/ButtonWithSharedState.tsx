import { useState } from "react";

type State = {
  sharedCount: number
  clbk: () => void
};

function ButtonWithSharedState({ sharedCount, clbk }: State) {
  console.log("ButtonWithSharedState has been called!");

  const [count, setCount] = useState<number>(0);

  return (
    <>
      <button onClick={() => {
        setCount(prevCount => prevCount + 1);
        setCount(prevCount => prevCount + 1);
      }
      }>
        increment +2 (internal state) ({count})
      </button>
      <button onClick={clbk}>
        increment +1 (shared state) ({sharedCount})
      </button>
    </>
  );
}

export default ButtonWithSharedState;
