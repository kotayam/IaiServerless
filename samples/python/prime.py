def isqrt(n):
    if n < 0:
        return 0
    x = n
    y = (x + 1) // 2
    while y < x:
        x = y
        y = (x + n // x) // 2
    return x

def is_prime(n):
    if n < 2:
        return False
    for i in range(2, isqrt(n) + 1):
        if n % i == 0:
            return False
    return True

def handler():
    # Find primes up to 100
    primes = [n for n in range(2, 101) if is_prime(n)]
    print("Primes up to 100:", primes)
    print("Count:", len(primes))
    return primes

