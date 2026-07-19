export interface DaylightCoordinates {
  latitude: number
  longitude: number
}

export interface AnalogDaylightWindow {
  sunriseMinute: number
  sunsetMinute: number
  approximate: boolean
}

export const fallbackSunriseMinute = 6 * 60
export const fallbackSunsetMinute = 18 * 60

const zenithDegrees = 90.833

function degreesToRadians(value: number): number {
  return value * Math.PI / 180
}

function radiansToDegrees(value: number): number {
  return value * 180 / Math.PI
}

function normalizeDegrees(value: number): number {
  return ((value % 360) + 360) % 360
}

function normalizeHours(value: number): number {
  return ((value % 24) + 24) % 24
}

function dayOfYear(date: Date): number {
  const start = Date.UTC(date.getFullYear(), 0, 0)
  const current = Date.UTC(date.getFullYear(), date.getMonth(), date.getDate())
  return Math.floor((current - start) / 86_400_000)
}

function calculateSunEventUtcHours(
  date: Date,
  coordinates: DaylightCoordinates,
  sunrise: boolean,
): number | null {
  const longitudeHour = coordinates.longitude / 15
  const approximateTime = dayOfYear(date)
    + ((sunrise ? 6 : 18) - longitudeHour) / 24
  const meanAnomaly = 0.9856 * approximateTime - 3.289
  const trueLongitude = normalizeDegrees(
    meanAnomaly
      + 1.916 * Math.sin(degreesToRadians(meanAnomaly))
      + 0.020 * Math.sin(degreesToRadians(2 * meanAnomaly))
      + 282.634,
  )

  let rightAscension = normalizeDegrees(
    radiansToDegrees(Math.atan(0.91764 * Math.tan(degreesToRadians(trueLongitude)))),
  )
  rightAscension += Math.floor(trueLongitude / 90) * 90
    - Math.floor(rightAscension / 90) * 90
  rightAscension /= 15

  const sinDeclination = 0.39782 * Math.sin(degreesToRadians(trueLongitude))
  const cosDeclination = Math.cos(Math.asin(sinDeclination))
  const cosHourAngle = (
    Math.cos(degreesToRadians(zenithDegrees))
      - sinDeclination * Math.sin(degreesToRadians(coordinates.latitude))
  ) / (
    cosDeclination * Math.cos(degreesToRadians(coordinates.latitude))
  )
  if (cosHourAngle < -1 || cosHourAngle > 1) {
    return null
  }

  const hourAngle = sunrise
    ? 360 - radiansToDegrees(Math.acos(cosHourAngle))
    : radiansToDegrees(Math.acos(cosHourAngle))
  const localMeanTime = hourAngle / 15
    + rightAscension
    - 0.06571 * approximateTime
    - 6.622
  return normalizeHours(localMeanTime - longitudeHour)
}

function validCoordinates(value: DaylightCoordinates | null): value is DaylightCoordinates {
  return value !== null
    && Number.isFinite(value.latitude)
    && Number.isFinite(value.longitude)
    && value.latitude >= -90
    && value.latitude <= 90
    && value.longitude >= -180
    && value.longitude <= 180
}

export function calculateAnalogDaylightWindow(
  date: Date,
  coordinates: DaylightCoordinates | null,
  utcOffsetMinutes = -date.getTimezoneOffset(),
): AnalogDaylightWindow {
  if (!validCoordinates(coordinates)) {
    return {
      sunriseMinute: fallbackSunriseMinute,
      sunsetMinute: fallbackSunsetMinute,
      approximate: true,
    }
  }

  const sunriseUtc = calculateSunEventUtcHours(date, coordinates, true)
  const sunsetUtc = calculateSunEventUtcHours(date, coordinates, false)
  if (sunriseUtc === null || sunsetUtc === null) {
    return {
      sunriseMinute: fallbackSunriseMinute,
      sunsetMinute: fallbackSunsetMinute,
      approximate: true,
    }
  }

  const toLocalMinute = (utcHours: number): number =>
    Math.round(normalizeHours(utcHours + utcOffsetMinutes / 60) * 60) % 1440
  const sunriseMinute = toLocalMinute(sunriseUtc)
  const sunsetMinute = toLocalMinute(sunsetUtc)
  if (sunriseMinute >= sunsetMinute) {
    return {
      sunriseMinute: fallbackSunriseMinute,
      sunsetMinute: fallbackSunsetMinute,
      approximate: true,
    }
  }
  return {
    sunriseMinute,
    sunsetMinute,
    approximate: false,
  }
}

export function normalizeDaylightCoordinates(value: unknown): DaylightCoordinates | null {
  if (typeof value !== 'object' || value === null || Array.isArray(value)) {
    return null
  }
  const source = value as Record<string, unknown>
  const coordinates = {
    latitude: Number(source.latitude),
    longitude: Number(source.longitude),
  }
  return validCoordinates(coordinates) ? coordinates : null
}
