import type { NextPage } from "next";
import { Chart } from "../components/Chart";
import styles from "../styles/Home.module.css";

const Home: NextPage = () => {
  return (
    <div className={styles.container}>
      <main className={styles.main}>
        <Chart />
        <section>
          <h2>Va?</h2>
          <p>
            <i>De som vet, de vet.</i> Kontakta Åke Amcoff (
            <a href="mailto:ake@amcoff.net">ake@amcoff.net</a>) vid frågor. Av
            sekretesskäl avslöjas inte mer här.
          </p>
        </section>
        <style jsx>{`
          section {
            margin: 0 8px;
          }

          @media (min-width: 480px) {
            section {
              margin: 0;
            }
          }
        `}</style>
      </main>
    </div>
  );
};

export default Home;
