import { InfluxDB } from "@influxdata/influxdb-client-browser";
import { DateTime, DateTimeUnit } from "luxon";
import useSWR, { SWRConfiguration, SWRResponse } from "swr";

export const influxClient = new InfluxDB({
  url: process.env.NEXT_PUBLIC_INFLUX_URL!,
  token: process.env.NEXT_PUBLIC_INFLUX_TOKEN!,
}).getQueryApi(process.env.NEXT_PUBLIC_INFLUX_ORG!);

interface Measurement {
  device: string;
  value: number;
  time: number;
  secondsSinceMidnight: number;
}

export function fetchMeasurements(
  start: string,
  stop = "now()"
): Promise<Measurement[]> {
  return new Promise((resolve, reject) => {
    const measurements: Measurement[] = [];

    const fluxQuery = `
    from(bucket: "${process.env.NEXT_PUBLIC_INFLUX_BUCKET}")
      |> range(start: ${start}, stop: ${stop})
      |> filter(fn: (r) => r._field == "prps")
      |> timedMovingAverage(every: 2m, period: 10m)
    `;

    influxClient.queryRows(fluxQuery, {
      next: (row, tableMeta) => {
        const o = tableMeta.toObject(row);
        const time = DateTime.fromISO(o._time);

        measurements.push({
          device: o.device,
          value: o._value,
          time: time.toMillis(),
          secondsSinceMidnight:
            time.hour * 3600 + time.minute * 60 + time.second,
        });
      },
      error: reject,
      complete: () => {
        resolve(measurements);
      },
    });
  });
}

export async function fetchMax(start: string, stop = "now()"): Promise<number> {
  const fluxQuery = `
  from(bucket: "${process.env.NEXT_PUBLIC_INFLUX_BUCKET}")
    |> range(start: ${start}, stop: ${stop})
    |> filter(fn: (r) => r._field == "prps")
    |> timedMovingAverage(every: 2m, period: 10m)
    |> max()
  `;

  const [res] = await influxClient.collectRows(fluxQuery);

  return (res as any)._value;
}

export const useMeasurements = (
  start = "-1d",
  stop = "now()",
  config?: SWRConfiguration
): SWRResponse<Measurement[]> => {
  return useSWR(
    `/measurements?start=${start}&stop=${stop}`,
    () => fetchMeasurements(start, stop),
    config
  );
};

export const useMax = (
  start = "-1d",
  stop = "now()",
  config?: SWRConfiguration
): SWRResponse<number> => {
  return useSWR(`/max?start=${start}&stop=${stop}`, () => fetchMax(start, stop), config);
}
