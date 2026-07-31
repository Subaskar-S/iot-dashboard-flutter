import 'package:flutter/foundation.dart';

@immutable
class Device {
  const Device({
    required this.id,
    required this.name,
    required this.type,
    required this.protocol,
    required this.address,
    required this.online,
  });

  final String id;
  final String name;
  final String type;
  final String protocol;
  final String address;
  final bool online;

  Device copyWith({
    String? id,
    String? name,
    String? type,
    String? protocol,
    String? address,
    bool? online,
  }) =>
      Device(
        id: id ?? this.id,
        name: name ?? this.name,
        type: type ?? this.type,
        protocol: protocol ?? this.protocol,
        address: address ?? this.address,
        online: online ?? this.online,
      );

  @override
  bool operator ==(Object other) =>
      identical(this, other) || other is Device && other.id == id;

  @override
  int get hashCode => id.hashCode;
}
