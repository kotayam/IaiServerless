def is_prime(n):
    if n < 2:
        return False
    for i in range(2, int(n ** 0.5) + 1):
        if n % i == 0:
            return False
    return True

# Find primes up to 100
primes = [n for n in range(2, 101) if is_prime(n)]
print("Primes up to 100:", primes)
print("Count:", len(primes))
