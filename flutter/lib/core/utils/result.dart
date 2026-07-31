/// Simple Result type — mirrors the C++ backend's std::expected pattern.
sealed class Result<T> {
  const Result();
}

final class Success<T> extends Result<T> {
  const Success(this.data);
  final T data;
}

final class Failure<T> extends Result<T> {
  const Failure(this.message, {this.statusCode});
  final String message;
  final int? statusCode;
}

extension ResultX<T> on Result<T> {
  bool get isSuccess => this is Success<T>;
  bool get isFailure => this is Failure<T>;

  T get data => (this as Success<T>).data;
  String get error => (this as Failure<T>).message;

  R fold<R>(R Function(T) onSuccess, R Function(String) onFailure) =>
      switch (this) {
        Success<T> s => onSuccess(s.data),
        Failure<T> f => onFailure(f.message),
      };
}
