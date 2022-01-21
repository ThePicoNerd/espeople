import { InfluxDB, Point } from "@influxdata/influxdb-client-browser";
import useSWR from "swr";

export const influxClient = new InfluxDB({
  url: process.env.NEXT_PUBLIC_INFLUX_URL!,
  token: process.env.NEXT_PUBLIC_INFLUX_TOKEN!,
}).getQueryApi(process.env.NEXT_PUBLIC_INFLUX_ORG!);

interface Measurement {
  device: string;
  value: number;
  time: number;
  movingAverage: number | null;
}

export function fetchMeasurements(
  range: string,
  includePrevious = 5
): Promise<Measurement[]> {
  return new Promise((resolve, reject) => {
    const measurements: Measurement[] = [];

    const fluxQuery = `
    from(bucket: "${process.env.NEXT_PUBLIC_INFLUX_BUCKET}")
    |> range(start: ${range})
    |> filter(fn: (r) => r._field == "prps")`;

    influxClient.queryRows(fluxQuery, {
      next: (row, tableMeta) => {
        const o = tableMeta.toObject(row);

        const previousMeasurements = measurements
          .slice(1)
          .slice(-includePrevious);
        const movingAverage =
          previousMeasurements.length === includePrevious
            ? (previousMeasurements.reduce((acc, cur) => acc + cur.value, 0) +
                o._value) /
              (includePrevious + 1)
            : null;

        measurements.push({
          device: o.device,
          value: o._value,
          time: new Date(o._time).getTime(),
          movingAverage,
        });
      },
      error: reject,
      complete: () => {
        resolve(measurements);
      },
    });
  });
}

export const useMeasurements = (range = "-1d") => {
  return useSWR(
    `/measurements?range=${range}`,
    () => fetchMeasurements(range),
    {
      refreshInterval: 10000,
    }
  );
};
