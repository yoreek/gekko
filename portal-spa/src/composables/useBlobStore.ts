import { ref, type Ref } from 'vue'
import { deleteBlob, fetchBlob, uploadBlob } from '@/api'

export interface UseBlobStoreReturn {
  busy: Ref<boolean>
  errorMessage: Ref<string>
  upload(prefix: string, bytes: Blob | ArrayBuffer): Promise<string>
  fetchBytes(key: string): Promise<Blob>
  remove(key: string): Promise<void>
}

// Thin wrapper over the generic blob-store REST API (docs/blob-store.md) - deliberately not
// display-specific, so any future feature that needs key/blob storage can reuse it instead of
// calling the client.ts functions directly.
export function useBlobStore(): UseBlobStoreReturn {
  const busy = ref(false)
  const errorMessage = ref('')

  async function upload(prefix: string, bytes: Blob | ArrayBuffer): Promise<string> {
    busy.value = true
    errorMessage.value = ''
    try {
      const response = await uploadBlob(prefix, bytes)
      return response.key
    } catch (error) {
      errorMessage.value = formatError(error)
      throw error
    } finally {
      busy.value = false
    }
  }

  async function fetchBytes(key: string): Promise<Blob> {
    busy.value = true
    errorMessage.value = ''
    try {
      return await fetchBlob(key)
    } catch (error) {
      errorMessage.value = formatError(error)
      throw error
    } finally {
      busy.value = false
    }
  }

  async function remove(key: string): Promise<void> {
    busy.value = true
    errorMessage.value = ''
    try {
      await deleteBlob(key)
    } catch (error) {
      errorMessage.value = formatError(error)
      throw error
    } finally {
      busy.value = false
    }
  }

  return { busy, errorMessage, upload, fetchBytes, remove }
}

function formatError(error: unknown): string {
  if (error instanceof Error) {
    return error.message
  }
  return 'Unknown error'
}
