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

export async function requestJson<T>(path: string, options: RequestOptions = {}): Promise<T> {
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
  } finally {
    window.clearTimeout(timeout)
  }
}

export async function requestForm<T>(path: string, form: URLSearchParams, options: RequestOptions = {}): Promise<T> {
  return requestJson<T>(path, {
    ...options,
    method: options.method ?? 'POST',
    body: form.toString(),
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8',
      ...(options.headers ?? {}),
    },
  })
}

export async function requestEmpty(path: string, options: RequestOptions = {}): Promise<void> {
  await requestJson<undefined>(path, options)
}
