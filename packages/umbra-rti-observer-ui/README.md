# Umbra Runtime Observer UI

The React/TypeScript frontend for the provider-neutral Umbra runtime observer.
It consumes only the observer HTTP contract and can therefore be used with a
Python, Java, or vendor-specific backend that implements that contract.

## Development

Start an observer backend on `http://127.0.0.1:8765`, then run:

```powershell
pnpm install
pnpm generate:api
pnpm dev
```

The Vite development server proxies `/api` and `/ws` to the Python showcase.

`generate:api` uses the FastAPI-generated `/openapi.json` document to refresh
`src/api/generated.ts`. The generated file is a compile-time client contract;
the backend remains responsible for runtime validation with Pydantic or its
language-equivalent models.

Build the UI for an observer service with:

```powershell
pnpm build
```

Then point `UMBRA_OBSERVER_UI_DIST` at this package's `dist` directory when
starting the Python service. The service falls back to its zero-build HTML
dashboard when the React build is not present.
