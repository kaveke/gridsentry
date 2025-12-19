FROM public.ecr.aws/lambda/python:3.12

# Set working directory
WORKDIR /var/task

# Copy requirements and install
COPY requirements.txt .
RUN pip install -r requirements.txt --target .

# Copy application code
COPY app/ ./

# Set Lambda handler
CMD ["lambda_function.lambda_handler"]
