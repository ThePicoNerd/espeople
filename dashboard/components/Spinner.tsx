import { FunctionComponent } from "react";

export const Spinner: FunctionComponent = () => (
  <div className="spinner">
    <style jsx>{`
      .spinner {
        width: 16px;
        height: 16px;
        border: 2px solid #555;
        border-radius: 100%;
        border-right-color: #777;
        animation: spin 0.75s linear infinite;
      }

      @keyframes spin {
        0% {
          transform: rotate(0deg);
        }

        100% {
          transform: rotate(360deg);
        }
      }
    `}</style>
  </div>
);
