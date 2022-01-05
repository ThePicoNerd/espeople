use actix_web::{post, web, App, HttpResponse, HttpServer, Responder};
use serde::{Serialize, Deserialize};
use std::fs::{OpenOptions, File};
use std::io::prelude::*;
use std::path::Path;
use chrono::Utc;

#[derive(Debug, Serialize, Deserialize)]
struct Datapoint {
    packet_count: u16,
    duration: u16,
}

#[post("/insert")]
async fn insert(info: web::Json<Datapoint>) -> impl Responder {
    println!("{} packets in {} milliseconds", info.packet_count, info.duration);

    let path = Path::new("data.csv");

    if !path.exists() {
        let mut file = File::create(path).unwrap();
        file.write_all(b"timestamp,packet_count,duration\n").unwrap();
    }

    let mut file = OpenOptions::new()
        .write(true)
        .append(true)
        .open(path)
        .unwrap();

    writeln!(file, "{},{},{}", Utc::now().to_rfc3339(), info.packet_count, info.duration).unwrap();

    HttpResponse::Ok().body("OK!")
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    HttpServer::new(|| {
        App::new()
            .service(insert)
    })
    .bind("0.0.0.0:8000")?
    .run()
    .await
}
