import { DateTime } from "luxon";
import { useEffect, useState } from "react";

export function useTime(refreshInterval = 1000) {
  const [time, setTime] = useState(DateTime.now);
  useEffect(() => {
    const timer = setInterval(() => {
      setTime(DateTime.now);
    }, refreshInterval);

    return () => {
      clearInterval(timer);
    };
  }, [refreshInterval]);

  return time;
}
