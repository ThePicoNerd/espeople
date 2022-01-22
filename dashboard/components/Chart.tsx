import { FunctionComponent, useState } from "react";
import {
  CartesianGrid,
  Legend,
  Line,
  LineChart,
  ReferenceLine,
  ResponsiveContainer,
  XAxis,
} from "recharts";
import { DateTime } from "luxon";
import { useMeasurements } from "../lib/data";
import { Spinner } from "./Spinner";
import { useTime } from "../lib/use-time";

function secsToHHmm(secs: number) {
  const hours = Math.floor(secs / 3600) % 24;
  const minutes = Math.floor((secs % 3600) / 60);
  return `${hours.toString().padStart(2, "0")}:${minutes
    .toString()
    .padStart(2, "0")}`;
}

export const Chart: FunctionComponent = () => {
  const now = useTime(5000);
  const yesterday = now.minus({ days: 1 });
  const lastWeek = now.minus({ days: 7 });

  const { data: yesterdaysData } = useMeasurements(
    yesterday.startOf("day").toISO(),
    yesterday.endOf("day").toISO(),
    { revalidateOnFocus: false }
  );
  const { data: lastWeeksData } = useMeasurements(
    lastWeek.startOf("day").toISO(),
    lastWeek.endOf("day").toISO(),
    { revalidateOnFocus: false }
  );
  const { data: liveData, isValidating } = useMeasurements(
    now.startOf("day").toISO(),
    now.endOf("day").toISO(),
    {
      refreshInterval: 10000,
    }
  );

  return (
    <div className="card">
      <div className="spinner-container">{isValidating && <Spinner />}</div>
      <h2>Aktivitet</h2>
      <ResponsiveContainer width="100%" height="100%" minHeight={400}>
        <LineChart
          margin={{ top: 0, right: 0, bottom: 0, left: 0 }}
          data={liveData}
        >
          <XAxis
            dataKey="secondsSinceMidnight"
            tickFormatter={secsToHHmm}
            type="number"
            stroke="#666"
            scale="time"
            interval="preserveStartEnd"
            tick={{ fontSize: 12, fontFamily: "inherit" }}
            ticks={Array.from({ length: 25 }).map((_, i) => i * 3600)}
          />
          <CartesianGrid stroke="#666" strokeWidth={0.5} />
          <Line
            type="monotone"
            dataKey="value"
            stroke="#888"
            dot={false}
            data={yesterdaysData}
            strokeWidth={0.5}
            name="I går"
            id="yesterday"
          />
          <Line
            type="monotone"
            dataKey="value"
            stroke="#8884d8"
            dot={false}
            data={lastWeeksData}
            strokeWidth={0.5}
            strokeDasharray={5}
            name="Förra veckan"
            id="lastWeek"
          />
          <Line
            type="monotone"
            dataKey="value"
            stroke="#8884d8"
            data={liveData}
            dot={false}
            name="Live"
            id="live"
          />
          <ReferenceLine
            x={now.hour * 3600 + now.minute * 60 + now.second}
            label={now.toLocaleString(DateTime.TIME_SIMPLE)}
            stroke="#666"
            strokeWidth={0.5}
            strokeDasharray={10}
            style={{ fill: "#fff" }}
            fill="#fff"
          />
          <Legend iconType="plainline" />
        </LineChart>
      </ResponsiveContainer>
      <style jsx>{`
        .card {
          width: 100%;
          background-color: #111;
          padding-block: 16px;
          padding-inline: 8px;
          position: relative;
        }

        @media (min-width: 480px) {
          .card {
            border: 1px solid #222;
            border-radius: 12px;
            padding-inline: 16px;
          }
        }

        h2 {
          text-align: center;
        }

        .spinner-container {
          position: absolute;
          top: 8px;
          left: 8px;
        }

        .card :global(.recharts-legend-item) {
          font-size: 13px;
          font-style: italic;
        }

        .card :global(.recharts-label) {
          fill: #aaa;
          font-family: "IBM Plex Mono", monospace;
          font-size: 13px;
        }
      `}</style>
    </div>
  );
};
