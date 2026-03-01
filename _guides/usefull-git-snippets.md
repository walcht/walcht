---
title: Usefull Git (and GitHub) Snippets
short_desc: A list of Git commands that have proven very usefull and that I keep forgetting how to use again.
layout: home
---

* Do not remove this line (it will not be displayed)
{:toc}

# Git Snippets

## Check Out PR Locally and Push Changes Back to PR

1. You should have been given permission rights to edit the PR's fork by the PR
owner (otherwise best approach is to ask them or to ).

1. Fetch the reference to the PR into a new branch named `LOCAL_PR_BRANCH_NAME`:

  ```bash
  git fetch origin pull/<ID>/head:<LOCAL_PR_BRANCH_NAME>
  ```
  
  Note: <ID> is the PR's ID which you can find on GitHub (or whichever platform
  your are using). It should be something like this: #<integer>

1. Do some changes, commit them, then push back to PR using:

  ```bash
  git push <pr-repo.git> <LOCAL_PR_BRANCH_NAME>:<pr-branch-name>
  ```

## Push Changes to PR Without Write Permissions

I advise against doing this but in case you are left with no other choice:

1. Fork the original fork and clone it locally
1. Do the changes and commit them to some branch
1. Open a new PR against the orginal PR's branch (use GitHub's UI for that)
