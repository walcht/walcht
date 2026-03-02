function BasicButtonEvent() {
  /* you can pass event handlers to 'on*' events - you, obviously, should pass
  * is a clbk/function and NOT call it whithin (if you set up your language
  * server on your IDE then you should get an error in case you do so) */
  return (
    <button onClick={() => alert("I SAID NO CLICK U IDIOT!")}>
      do NOT click me!
    </button>
  );
}

export default BasicButtonEvent;
