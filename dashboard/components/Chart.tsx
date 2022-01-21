import { FunctionComponent } from "react";
import {
  CartesianGrid,
  Line,
  LineChart,
  ResponsiveContainer,
  XAxis,
} from "recharts";
import { DateTime } from "luxon";
import { useMeasurements } from "../lib/data";
import { Spinner } from "./Spinner";

function formatTimestamp(millis: number): string {
  const date = DateTime.fromMillis(millis);

  if (date.hasSame(DateTime.local(), "day")) {
    return date.toFormat("HH:mm");
  }

  return date.toFormat("yyyy-MM-dd HH:mm");
}

export const Chart: FunctionComponent = () => {
  const { data, isValidating } = useMeasurements();

  return (
    <div className="card">
      <div className="spinner-container">{isValidating && <Spinner />}</div>
      <h2>Aktivitet</h2>
      <ResponsiveContainer width="100%" height="100%" minHeight={400}>
        <LineChart data={data}>
          <XAxis
            dataKey="time"
            tickFormatter={formatTimestamp}
            type="number"
            domain={["auto", "auto"]}
            scale="time"
            stroke="#666"
            tick={{ fontSize: 12, fontFamily: "inherit" }}
          />
          <CartesianGrid stroke="#666" strokeWidth={0.5} />
          {/* <Line
            type="monotone"
            dataKey="value"
            stroke="#888"
            dot={false}
            strokeWidth={0.5}
          /> */}
          <Line
            type="monotone"
            dataKey="movingAverage"
            stroke="#8884d8"
            dot={false}
          />
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
      `}</style>
    </div>
  );
};
