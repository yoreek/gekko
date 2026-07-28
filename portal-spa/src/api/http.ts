import { safeReadStorage, safeWriteStorage } from '@/utils/storage'

export class ApiClientError extends Error {
  code: string
  status: number
  payload: unknown

  constructor(message: string, code: string, status: number, payload: unknown) {
    super(message)
    this.name = 'ApiClientError'
    this.code = code
    this.status = status
    this.payload = payload
  }
}

const apiBaseStorageKey = 'gekko.apiBase'

function normalizeBase(value: string): string {
  return value.replace(/\/+$/, '')
}

export function resolveApiBase(): string {
  if (typeof window === 'undefined') {
    return ''
  }

  const queryBase = new URL(window.location.href).searchParams.get('apiBase')
  const storedBase = safeReadStorage(apiBaseStorageKey)
  const envBase = (import.meta.env.VITE_API_BASE as string | undefined) ?? ''
  const base = queryBase ?? storedBase ?? envBase

  return base ? normalizeBase(base.trim()) : ''
}

export function setApiBase(value: string): void {
  safeWriteStorage(apiBaseStorageKey, normalizeBase(value.trim()))
}

function withBase(path: string, baseUrl = resolveApiBase()): string {
  if (/^https?:\/\//i.test(path)) {
    return path
  }
  if (!baseUrl) {
    return path
  }
  if (path.startsWith('/')) {
    return `${baseUrl}${path}`
  }
  return `${baseUrl}/${path}`
}

export type RequestMethod = 'GET' | 'POST' | 'PUT' | 'PATCH' | 'DELETE'

export interface RequestOptions {
  method?: RequestMethod
  body?: BodyInit | null
  headers?: HeadersInit
  timeoutMs?: number
}

async function requestWithTextResponse(
  path: string,
  options: Omit<RequestOptions, 'body'> & { body?: BodyInit | FormData | null } = {},
): Promise<{ response: Response; text: string }> {
  const controller = new AbortController()
  const timeoutMs = options.timeoutMs ?? 10000
  const timeout = window.setTimeout(() => controller.abort(), timeoutMs)

  try {
    const response = await fetch(withBase(path), {
      method: options.method ?? 'GET',
      body: options.body ?? null,
      headers: {
        Accept: 'application/json',
        ...(options.headers ?? {}),
      },
      signal: controller.signal,
    })

    const text = await response.text()
    return { response, text }
  } finally {
    window.clearTimeout(timeout)
  }
}

function parseJsonResponse<T>(response: Response, text: string): T {
  const payload = text.length > 0 ? JSON.parse(text) : undefined

  if (!response.ok) {
    const code = typeof payload === 'object' && payload !== null && 'code' in payload ? String((payload as { code: unknown }).code) : 'HTTP_ERROR'
    const message = typeof payload === 'object' && payload !== null && 'error' in payload ? String((payload as { error: unknown }).error) : response.statusText
    throw new ApiClientError(message, code, response.status, payload)
  }

  if (payload && typeof payload === 'object' && 'success' in payload && (payload as { success?: boolean }).success === false) {
    const code = 'code' in payload ? String((payload as { code: unknown }).code) : 'API_ERROR'
    const message = 'error' in payload ? String((payload as { error: unknown }).error) : 'request failed'
    throw new ApiClientError(message, code, response.status, payload)
  }

  return payload as T
}

export async function requestJson<T>(path: string, options: RequestOptions = {}): Promise<T> {
  const { response, text } = await requestWithTextResponse(path, options)
  return parseJsonResponse<T>(response, text)
}

export async function requestEmpty(path: string, options: RequestOptions = {}): Promise<void> {
  await requestJson<undefined>(path, options)
}

export async function requestText(path: string, options: RequestOptions = {}): Promise<string> {
  const { response, text } = await requestWithTextResponse(path, options)
  if (!response.ok) {
    throw new ApiClientError(response.statusText || 'request failed', 'HTTP_ERROR', response.status, text)
  }
  return text
}

export async function requestFormData<T>(path: string, formData: FormData, options: RequestOptions = {}): Promise<T> {
  const { response, text } = await requestWithTextResponse(path, {
    ...options,
    body: formData,
    method: options.method ?? 'POST',
  })
  return parseJsonResponse<T>(response, text)
}

// Sends a raw binary body (no multipart/JSON wrapping) - used by the generic blob-store endpoints
// (/api/blobs/...), which read the request body as-is.
export async function requestBinary<T>(path: string, body: BodyInit, options: RequestOptions = {}): Promise<T> {
  const { response, text } = await requestWithTextResponse(path, {
    ...options,
    body,
    method: options.method ?? 'POST',
  })
  return parseJsonResponse<T>(response, text)
}

// Fetches a raw binary response body as a Blob rather than parsing it as JSON - used to read a
// blob's bytes back from /api/blobs/<key>.
export async function requestBlob(path: string, options: Omit<RequestOptions, 'body'> = {}): Promise<Blob> {
  const controller = new AbortController()
  const timeoutMs = options.timeoutMs ?? 10000
  const timeout = window.setTimeout(() => controller.abort(), timeoutMs)

  try {
    const response = await fetch(withBase(path), {
      method: options.method ?? 'GET',
      headers: options.headers,
      signal: controller.signal,
    })
    if (!response.ok) {
      throw new ApiClientError(response.statusText || 'request failed', 'HTTP_ERROR', response.status, null)
    }
    return await response.blob()
  } finally {
    window.clearTimeout(timeout)
  }
}
