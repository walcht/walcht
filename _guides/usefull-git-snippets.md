---
title: Usefull Git (and GitHub) Snippets
short_desc: A list of Git commands that have proven very usefull and that I keep forgetting how to use again.
layout: home
show_edit_source: true
add_no_ai_banner: true
---

* Do not remove this line (it will not be displayed)
{:toc}

# Git Snippets

Do not worry. Github CLI (or other *non-FOSS*) tools will not be mentioned. All
you need is a modern version of Git. And a terminal.

## Change Default Editor to Nano

If you use Neovim with integrated terminal then you will thank me for mentioning
this:

```bash
git config --global core.editor "nano"
```

## Github and Why you Should Probably Stop Using It 

Just read this: [GiveUpGitHub](https://sfconservancy.org/GiveUpGitHub/).

I am not a fan of GitHub and especially not a fan of GitHub actions and how
bloated that CI/CD automation tool is (I also hold a grudge against it, because
some recruiter didn't notice on my CV and thought that I am not *qualified
enough* for the role...).

Letting LLMs roam free and flood repos with PRs (also issues, general messages,
etc.) is just something I don't have the time nor the will to put up with.

Suffice to say that the only reason I am using GitHub is the same reason why I
am still using Facebook: **the network effect**. Friends and family use,
therefore I also have to use it. Developers and recruiters use it, therefore I
have to use it.

## Commit Messages

### Revert/Edit the Latest Commit Message Because You Apparently Can't Write Properly Anymore

```bash
git commit --amend
```

## Pull Requests

### Check Out PR Locally and Push Changes Back to PR

1. You should have been given permission rights to edit the PR's fork by the PR
owner (otherwise best approach is to ask them or if you can't then take look at
[Push Changes to PR Without Write Permissions](#Push-Changes-to-PR-Without-Write-Permissions)).

1. Fetch the reference to the PR into a new branch named `LOCAL_PR_BRANCH_NAME`:

   ```bash
   git fetch origin pull/<ID>/head:<LOCAL_PR_BRANCH_NAME>
   ```
   
   Note: <ID> is the PR's ID which you can find on GitHub (or whichever platform
   your are using). It should be something like this: `#<integer>`

1. Do some changes, commit them, then push back to PR using:

   ```bash
   git push <pr-repo.git> <LOCAL_PR_BRANCH_NAME>:<pr-branch-name>
   ```

### Push Changes to PR Without Write Permissions

I advise against doing this but in case you are left with no other choice:

1. Fork the original fork and clone it locally (use Github's UI for the fork
and: `git clone <FORKED_REPO.git>`)
1. Do the changes and commit them to some branch
1. Open a new PR against the orginal PR's branch (use GitHub's UI for that)

### PRs and Merge Conflicts

Say you have two PRs: PR0 and PR1. PR0 was opened **before** PR1 but PR1 got
meged first. Assume that PR0 now has merge conflicts with the master (or main)
branch because of the just-merged PR1. Sure, you can ask PR0's owner to handle
the merge conflicts but what if you want to do it yourself as the repo's
contributor?


1. Add orginal repository to remote and name it `upstream`:

   ```bash
   git remote add upstream git@github.com:<ORIGINAL_AUTHOR>/<PROJECT_NAME>.git
   ```

1. Fetch the commits and branches from `upstream`:

   ```bash
   git fetch upstream
   ```

1. Checkout your master (or main) branch (i.e., your fork):

   ```bash
   git checkout master
   ```

1. Merge the changes from the orginal repository (i.e., `upstream`) into your
master (or main) branch:

   ```bash
   git merge upstream/master
   ```

1. Fix potential merge conflicts and commit your changes:

   ```bash
   git commit -am "commit message"
   ```

1. Finally, push changes to your fork:

   ```bash
   git push
   ```

Your fork is now in sync with the original repository and any potential PRs
(obviously only PRs that are against the branch we synced - master) can be
merged without conflicts (unless another PR gets merged again before yours in
which case you have to redo all of this shit).
