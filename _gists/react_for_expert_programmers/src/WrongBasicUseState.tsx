import { useState } from "react";

function WrongBasicUseState() {
  /* log - so that we know when this is getting called */
  console.log("WrongBasicUseState has been called!");

  /* useState returns a stateful value and a function to update it. Notice the
  * 'const' identifier here. We pass the initial value to useState - here it is
  * a 0. I prefer the 'generic' call to be explicit about the type of the state
  * I want (and also to be friendly to other programmers - e.g., C++) */
  const [count, setCount] = useState<number>(0);

  /* this is the wrong way to call setCount - double increment wont work */
  return (
    <>
      <button onClick={() => {
        setCount(count + 1);  // remember, count is CONST, it wont be updated!
        setCount(count + 1);  // .. still taking old count
      }
      }>
        increment two times ({count})
      </button>
    </>
  );
}

export default WrongBasicUseState;
