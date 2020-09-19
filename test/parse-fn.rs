fn factorial(i: u64) -> u64 {
    return match i {
        0 => 1,
        n => n * factorial(n-1)
    };
}
