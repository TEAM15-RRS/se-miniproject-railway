# Git / GitHub setup - first 15 minutes

```bash
# 1. ONE person creates an empty repo on GitHub (no README), adds the other 3 as collaborators.
# 2. Same person, inside this folder:
git init -b main
git add .
git commit -m "chore: initial project skeleton, core modules, tests and CI"
git remote add origin https://github.com/<org>/<repo>.git
git push -u origin main
git switch -c develop && git push -u origin develop

# 3. Everyone else:
git clone https://github.com/<org>/<repo>.git && cd <repo>
git config user.name  "Your Name"
git config user.email "the-email-linked-to-your-github-account"   # otherwise commits are not credited to you!

# 4. Day-to-day
git switch develop && git pull
git switch -c feature/search-edge-cases
#   ...edit, run `make test`...
git add -A && git commit -m "test(search): cover same-station and past-date errors (#4)"
git push -u origin feature/search-edge-cases      # then open a Pull Request into develop
```

GitHub settings worth doing (Settings -> Branches -> Add rule for `main`): require a pull request and require the
**CI** status check to pass before merging. Then "resolve build failures before moving on" is enforced automatically.

Create issues: `gh auth login && ./scripts/create_issues.sh`
