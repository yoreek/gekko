async function loadImage(url: string): Promise<HTMLImageElement> {
  return new Promise((resolve, reject) => {
    const image = new Image()
    image.onload = () => resolve(image)
    image.onerror = () => reject(new Error('Unable to decode image'))
    image.src = url
  })
}

export async function decodeBitmapFile(file: File): Promise<ImageBitmap> {
  if ('createImageBitmap' in globalThis) {
    return await globalThis.createImageBitmap(file)
  }
  const url = URL.createObjectURL(file)
  try {
    const image = await loadImage(url)
    const canvas = document.createElement('canvas')
    canvas.width = image.naturalWidth
    canvas.height = image.naturalHeight
    const context = canvas.getContext('2d')
    if (context === null) {
      throw new Error('Canvas 2D context is unavailable')
    }
    context.drawImage(image, 0, 0)
    return await createImageBitmap(canvas)
  } finally {
    URL.revokeObjectURL(url)
  }
}
