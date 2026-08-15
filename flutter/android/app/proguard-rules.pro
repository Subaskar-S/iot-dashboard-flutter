# Flutter's own ProGuard rules are contributed by the Flutter Gradle plugin.
# R8 is enabled in release (isMinifyEnabled) to shrink the bundle.

# flutter_secure_storage
-keep class androidx.security.crypto.** { *; }

# Keep annotation metadata used by json_serializable-generated code paths.
-keepattributes *Annotation*, Signature, InnerClasses, EnclosingMethod
