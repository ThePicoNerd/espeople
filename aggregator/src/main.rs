use actix_web::{post, web, App, HttpResponse, HttpServer, Responder};
use serde::{Serialize, Deserialize};

#[derive(Debug, Serialize, Deserialize)]
struct Datapoint {
    packet_count: u16,
    duration: u16,
}

#[post("/insert")]
async fn insert(info: web::Json<Datapoint>) -> impl Responder {
    println!("{} packets in {} milliseconds", info.packet_count, info.duration);

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
