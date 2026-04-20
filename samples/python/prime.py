def is_prime(n):
    if n < 2:
        return False
    i = 2
    while i * i <= n:
        if n % i == 0:
            return False
        i += 1
    return True

def handler():
    n_str = input()
    n = 0
    for c in n_str:
        if c >= '0' and c <= '9':
            n = n * 10 + (ord(c) - ord('0'))
    if n <= 0:
        n = 10000
    count = 0
    num = 2
    last_prime = 2
    while count < n:
        if is_prime(num):
            count += 1
            last_prime = num
        num += 1
    print("Compute Task Complete. %dth prime is: %d" % (n, last_prime))

