# Simple script - runs directly
print("Hello from Python on IaiServerless!")
print("This is a standard Python program")

# Can also define a handler for serverless pattern
def handler():
    print("Handler called!")
    return "Success"
