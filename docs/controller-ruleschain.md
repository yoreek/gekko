# Controller And RulesChain Guide

This project now uses a controller-style HTTP layer for portal endpoints. The pattern is intentionally close to the `gekko/api/BaseController` flow so future portal features can stay consistent.

## Goals

- Keep HTTP behavior explicit and testable.
- Put shared request handling in `BaseController`.
- Keep per-domain logic in focused controller classes.
- Use `RulesChain` hooks for preconditions and request preparation.
- Keep CORS and no-cache behavior in the preflight hook path, not scattered across handlers.

## Core Pieces

### `BaseController`

`BaseController` owns the common HTTP lifecycle:

- action dispatch
- `OPTIONS` preflight handling
- bounded JSON body parsing
- standard success and error envelopes
- JSON response sending

It does not know any domain-specific rules.

### `RulesChain`

`RulesChain` is the hook system used before action dispatch.

Each controller can provide a chain of rules from `beforeChain()`. A rule contains:

- a hook function: `bool (*)(BaseController& self)`
- an action mask that says when the hook should run

The chain is recursive:

- the base chain runs first
- the derived controller chain runs after it
- hook execution stops on the first `false`

## Current Base Behavior

The shared base chain currently contains:

- `beforeCorsOptions` for `OPTIONS` preflight
- `parseBody` for actions that need JSON input

That means:

- CORS and no-cache headers are applied by the preflight hook
- body parsing happens before create/update/command actions
- controllers do not need to repeat those concerns in every action

## How To Add A New Controller

Use this structure:

```cpp
class MyController : public BaseController {
public:
    MyController(AsyncWebServerRequest* request, Action action, MyDomain& domain)
        : BaseController(request, action), domain_(domain) {}

protected:
    const RulesChain* beforeChain() override;
    void index() override;
    void show() override;
    void create() override;

private:
    MyDomain& domain_;
    static bool requireId(BaseController& self);
    static bool requireRecord(BaseController& self);
};
```

Then define the chain:

```cpp
const BaseController::RulesChain* MyController::beforeChain() {
    static constexpr HookRule rules[] = {
        {&MyController::requireId, A(Action::Show) | A(Action::Destroy) | A(Action::Cmd)},
        {&MyController::requireRecord, A(Action::Show) | A(Action::Destroy) | A(Action::Cmd)},
    };
    static const RulesChain node{rules, sizeof(rules) / sizeof(rules[0]), BaseController::beforeChain()};
    return &node;
}
```

## Hook Rules

### Use hooks for preconditions

Good hook examples:

- parse an id from the request path
- resolve a record from a registry
- reject a request with `400` or `404` before the action runs
- validate that a path-specific command suffix is present

### Do not use hooks for unrelated response decoration

Do not add response headers from arbitrary controllers or actions.

In this project:

- `beforeCorsOptions` owns CORS preflight handling
- `beforeCorsOptions` also owns the shared no-cache header behavior for the `OPTIONS` response
- normal action methods should only produce the domain response body

### Keep hook functions static

Hook functions should be:

```cpp
static bool hookName(BaseController& self);
```

That keeps them compatible with `HookFn` and avoids binding a controller instance into the rule table.

### Keep hook logic bounded

Hook logic should:

- inspect request path, action, or body
- resolve cached state from the controller or registry
- fail fast with a JSON error if the request is invalid

Hook logic should not:

- start or stop unrelated hardware
- allocate unbounded memory
- perform multi-step side effects that belong in the action body

## Path Parsing Pattern

For routes like `/api/devices/:id` and `/api/devices/:id/command`, use a dedicated path parser in the controller.

Recommended behavior:

- `show`, `destroy`, and other id-based actions accept `/api/devices/<id>`
- `cmd` requires `/api/devices/<id>/command`
- invalid ids return `400 BAD_PARAMS`
- missing records return `404 NOT_FOUND`

This keeps the route contract explicit and avoids accidental matching of the wrong action.

## Body Parsing Pattern

For actions that expect JSON:

- let the base chain parse the body before the action runs
- let the controller action assume the body exists and is already validated
- keep the validation error path bounded and consistent

If an action does not need a body, do not add it to the mask.

## Response Pattern

Use the shared response helpers:

- `renderOk(doc)` for success responses that should include `success: true` and `status: "ok"`
- `renderError(httpCode, code, message)` for failures
- `sendJson(httpCode, doc)` when the envelope is already built manually

The project standard is:

- success envelope: `{"success":true,...}`
- error envelope: `{"success":false,"code":"...","error":"..."}`

## CORS And OPTIONS

The `OPTIONS` flow is handled in `beforeCorsOptions`.

That hook should:

- detect `HTTP_OPTIONS`
- create a `204` response
- add CORS headers
- add no-cache headers
- send the response directly
- stop the action dispatch chain

That is the single place where preflight headers are applied.

## What To Reuse For New Features

When adding a new portal API feature:

1. Create a focused controller class.
2. Add a `beforeChain()` override.
3. Add hook functions for request-specific preconditions.
4. Keep body parsing in the shared base chain when the action needs JSON.
5. Keep CORS and no-cache behavior in the shared preflight hook.
6. Return standard envelopes from all responses.
7. Add tests for the path contract and error paths, not only the success path.

## Practical Example

If a feature has:

- `GET /api/widgets`
- `GET /api/widgets/:id`
- `POST /api/widgets/:id/command`

then the controller should usually:

- use an `Index` action for the collection path
- use `requireId` for the id-based actions
- use `requireEntity` for actions that need the loaded record
- use a stricter path parser for the command path if it has a suffix

This keeps the route contract aligned with the action name, not just the raw prefix.
