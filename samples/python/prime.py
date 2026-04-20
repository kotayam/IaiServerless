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
    n_str = input()
    n = 0
    for c in n_str:
        if c >= '0' and c <= '9':
            n = n * 10 + (ord(c) - ord('0'))
    if n <= 0:
        n = 100
    primes = [x for x in range(2, n + 1) if is_prime(x)]
    print("Primes up to %d: %d found, largest is %d" % (n, len(primes), primes[-1] if primes else 0))

