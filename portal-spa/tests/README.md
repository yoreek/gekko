# Tests

`tests/unit/` contains Node `node:test` specs for data, model, and math logic.
`tests/e2e/` contains Playwright specs for browser flows and live UI behavior.

Run them with:

```bash
pnpm --dir portal-spa test:unit
pnpm --dir portal-spa test:unit:coverage
pnpm --dir portal-spa smoke
```

Keep new test files in the matching folder so the runners stay separated:

- `tests/unit/*.spec.ts` for `node:test`
- `tests/e2e/*.spec.ts` for Playwright
