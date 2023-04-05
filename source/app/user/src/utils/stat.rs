use derive_more::{Deref, DerefMut};
use std::time::{Duration, Instant};

pub struct Stat {
    pub name: String,
    pub units: String,
    pub print_period: Duration,

    count: u64,
    sum: f64,
    max: f64,
    min: f64,

    last_print: Instant,
}

impl Stat {
    pub fn new(name: String) -> Self {
        Self {
            name,
            units: String::new(),
            print_period: Duration::from_secs(10),

            count: 0,
            sum: 0.0,
            max: f64::MIN,
            min: f64::MAX,

            last_print: Instant::now(),
        }
    }

    pub fn reset(&mut self) {
        self.count = 0;
        self.sum = 0.0;
        self.max = f64::MIN;
        self.min = f64::MAX;
    }

    pub fn sample(&mut self, value: f64) {
        self.sum += value;
        self.max = self.max.max(value);
        self.min = self.min.min(value);
        self.count += 1;

        let now = Instant::now();
        if now - self.last_print >= self.print_period {
            self.print();
            self.last_print = now;
        }
    }

    pub fn print(&self) {
        println!("stat '{}':", self.name);
        println!("  count: {}", self.count);
        println!("  avg: {} {}", self.sum / self.count as f64, self.units);
        println!("  min: {} {}", self.min, self.units);
        println!("  max: {} {}", self.max, self.units);
    }
}

#[derive(Deref, DerefMut)]
pub struct TimeStat {
    #[deref]
    #[deref_mut]
    stat: Stat,
    last_sample: Option<Instant>,
}

impl TimeStat {
    pub fn new(name: String) -> Self {
        let mut this = Self {
            stat: Stat::new(name),
            last_sample: None,
        };
        this.stat.units = "ms".into();
        this
    }

    pub fn sample(&mut self) {
        let now = Instant::now();
        if let Some(last) = self.last_sample {
            self.stat.sample((now - last).as_secs_f64() * 1000.0);
        }
        self.last_sample = Some(now);
    }
}
