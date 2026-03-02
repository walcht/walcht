---
title: A short introduction to React with TypeScript
short_desc: A short introduction to React with TypeScript
layout: home
show_edit_source: true
add_no_ai_banner: true
---

* Do not remove this line (it will not be displayed)
{:toc}

# Setup

Probably the easiest out-of-the-box way to get started is to use [vite][vite]
and setup your project via: `npm create vite@latest` => then choose React and
TypeScript (or choose JavaScript if you enjoy suffering).

(I know - we are starting off with weird boilerplate setup. For the moment let's
get as fast as possible into using React then you can use other methods for
setting up your initial project).

## Warnings

Before we starts, my experience with *frontend web development* (or web
development in general) has been... well... really bad. The web development
sphere is the one influenced the most by hype trains, over-engineered *new*
solutions that solve non-existing problems or problems that have been solved
decades ago, and unconstrained urge to release a new JS framework every
nanosecond and call devs who do not use it names...

That being said, what you will see is breath of fresh air. For most projects,
you probably do not need React (vanilla TypeScript is completely fine). But for
complex projects with highly dynamic content, React is, *apparently*, a good
and well established choice.

In this crash cource, we'll take a look at it without over-complicating anything
and without bying into any other features/products that despretely tries to
attach themselves to React usage (i.g., "you have to use this X with React
otherwise you are missing big time", etc.).

## Project Structure

React *extends* JavaScript/TypeScript (because of the integrated HTML markup
syntax) and therefore files usually (and preferably) have the .jsx (or .tsx for
TypeScript) extension.

If you `tree -I node_modules` you will get something similar to this:

```
.
├── eslint.config.js
├── index.html          -> main page html (entry point)
├── package.json        -> Node package dependencies for this project
├── package-lock.json   -> Describes exact dependency tree
├── public              -> contains assets
│   └── ...
├── README.md
├── src
│   ├── App.css
│   ├── App.tsx
│   ├── assets
│   │   └── ...
│   ├── index.css
│   └── main.tsx
├── tsconfig.app.json   -> TypeScript config (linting, formatting, etc.)
├── tsconfig.json       -> main TypeScript config (references other configs)
├── tsconfig.node.json
└── vite.config.ts      -> vite config file (add vite plugins here, etc.)
```

The `package.json` file essentially describes your Node package project
(remember that it is expected that a server will server this React project -
hence why Node is used). For instance, it lists the dependencies, the
development-only dependencies, it exposes cli commands through the `scripts`
entry, etc. See [package.json][package_json] for detailed description of each
field.

The `package-lock.json` *locks* and *stores* the exact dependencies at the time
of the last npm invocation that modifies the `node_modules` or `package_json`
tree. See [package-lock.json][package_lock] for detailed description.

## TypeScript Project Conventions

The naming convention of files in TypeScript projects *usually* (but does not
have to - this is why it's called a convention) follows the name of the main
componenet that is exported from that file. For instance, if we have a function
named `Foo` that holds a self-contained React component (we'll come to that
later), then it should preferably be put in a separate file under the name
`Foo.tsx`. That why any other file using that compoenent will do so as such:
`import Foo from './Foo';`.

## React Hello World

{% highlight tsx linenos %}

const user = {
  name: "walcht",
  imageUrl: "https://i.imgur.com/yXOvdOSs.jpg",
  imageSize: 90,
};

function App() {
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
    </>
  )
}

export default App;

{% endhighlight %}

## Handling a List of Items

Let's say we want to render a list of product items (like a shopping list or
whatever). Read the comments in the code below as there are a couple of new
concepts here (e.g., React attributes, automatic list expansion, etc.):

{% highlight tsx linenos %}

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

{% endhighlight %}

Then in `App.tsx` call the previous React compoenent as such:
`<ShoppingList />`.

## Handling Events

{% highlight tsx linenos %}

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

{% endhighlight %}

Then, as usual, call this compoenent in `App.tsx` as such:
`<BasicButtonEvent />`. You should now be familiar with this - in the remainer
of this guide, this will not be mentioned again (unless something changes).

## Stateful Components Using useState

Now we get to one of the actual power-features React adds (there is a claim that
this significantly improves Vanilla TS development experience - we will verify
this alongside this tutorial. Usually claims such as the above are very hard to
verify - if not impossible - but we will keep an open mind). Also worth noting
that I am always biased towards less complexity, less overhead, and less code
and therefore I am biased towards Vanilla TS unless React has provides a strong
valid reason to use it (usually complex and very dynamic websites).

I think you would agree that having components remember their state is a very
handy feature to have. Doing so in Vanilla TS *usually* involves ugly
declarations of global variables (sure, you can *wrap* components in classes
alongside their internal state - and there is actually a way to use *class
state* in React).

Do to so, we use the `useState` *hook* (we'll come back to what these are later
on):

{% highlight tsx linenos %}

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

{% endhighlight %}

As the name of the function suggests, this is the **wrong way to call
setCount**. Apparently, from initial observation, `setCount` should cause the
compoenent to get dirty and be redrawn (hence why we see the console logs - as
to why we them in a batch of twos - I still don't know). `setCount` gets as a
parameter the new value for its internal state which it **returns in the next
redraw execution**. The way we do it here is wrong because `setCount` is called,
then called again while still being supplied the same **const** `count` value.

To solve this, we can pass a function to `setCount` that takes the previous
state value and updates it:

{% highlight tsx linenos %}

// ...

setCount((prevCount) => prevCount + 1);
setCount((prevCount) => prevCount + 1);

// ...

{% endhighlight %}

Notice here that we are NOT passing the **const** `count` value but we are
essentially providing a mechanism (a function callback) to `setCount` for it to
provide us with its **actual up-to-date internal state**. If you come from a C
or C++ background then you should have noticed the two issues; firstly that
count is passed by value and there is no mechanism to pass a primitive value by
reference in JavaScript, and secondly that the web is a giant mess.


{% highlight tsx linenos %}



{% endhighlight %}

You can call the React component above as many times and you each component will
have its own internal state.

## Hooks? Like Fishing Hooks or What?

If you recall, `useState` is called a React *hook*. Actually anything that
starts with `use*` in React is a hook. Hooks have a particular set of strict
usage requirements:

 -. You cannot introduce any dyanmic control over the execution order of `use*`
 calls
 -. You have to only call them at the top of hook

## Sharing Data Between Components

Let's assume you want to share data between two Button components as dictated
in the diagram below:

{% highlight tsx linenos %}

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

{% endhighlight %}

Then create the shared state and pass it in the parent function (we assume it is
called `App` here):

{% highlight tsx linenos %}

function App() {

  /* create shared state here - count and clbk will be passed to child btns */
  const [count, setCount] = useState<number>(0);
  const clbk = () => { setCount(prevCount => prevCount + 1) };

  // ...

  return (
    <>
      <ButtonWithSharedState sharedCount={count} clbk={clbk} />
      <ButtonWithSharedState sharedCount={count} clbk={clbk} />
    </>
  )
}

export default App;

{% endhighlight %}

vite: https://vite.dev/guide/
package_json: https://docs.npmjs.com/cli/v10/configuring-npm/package-json
package_lock: https://docs.npmjs.com/cli/v10/configuring-npm/package-lock-json
