"""动态构建 protobuf 描述符，供 ProtobufImporter 与测试数据生成器共用。

不依赖 protoc / 预生成的 *_pb2.py：运行时用 FileDescriptorProto 构建消息类，
保证导入器与生成器使用同一 wire 格式。示例 schema 模拟一条监控采样消息：

    message MetricSample {
      int64  time_us      = 1;
      string host         = 2;
      double cpu_percent  = 3;
      double mem_percent  = 4;
      repeated CoreStat cores = 5;
    }
    message CoreStat {
      string core  = 1;
      double usage = 2;
    }
"""
from __future__ import annotations

from typing import Iterator

from google.protobuf import descriptor_pb2, descriptor_pool, message_factory
from google.protobuf.internal import decoder, encoder

MSG_TYPE = "pulseview.MetricSample"

_FD = descriptor_pb2.FieldDescriptorProto
_TYPE_DOUBLE = _FD.TYPE_DOUBLE
_TYPE_INT64 = _FD.TYPE_INT64
_TYPE_STRING = _FD.TYPE_STRING
_TYPE_MESSAGE = _FD.TYPE_MESSAGE
_LABEL_OPTIONAL = _FD.LABEL_OPTIONAL
_LABEL_REPEATED = _FD.LABEL_REPEATED

_pool = descriptor_pool.DescriptorPool()


def _add_field(msg, name, number, ftype, label=_LABEL_OPTIONAL, type_name=None):
    f = msg.field.add()
    f.name = name
    f.number = number
    f.type = ftype
    f.label = label
    if type_name:
        f.type_name = type_name


def _build_pool() -> None:
    fdp = descriptor_pb2.FileDescriptorProto()
    fdp.name = "pulseview_metric.proto"
    fdp.package = "pulseview"
    fdp.syntax = "proto3"

    core = fdp.message_type.add()
    core.name = "CoreStat"
    _add_field(core, "core", 1, _TYPE_STRING)
    _add_field(core, "usage", 2, _TYPE_DOUBLE)

    sample = fdp.message_type.add()
    sample.name = "MetricSample"
    _add_field(sample, "time_us", 1, _TYPE_INT64)
    _add_field(sample, "host", 2, _TYPE_STRING)
    _add_field(sample, "cpu_percent", 3, _TYPE_DOUBLE)
    _add_field(sample, "mem_percent", 4, _TYPE_DOUBLE)
    _add_field(sample, "cores", 5, _TYPE_MESSAGE, _LABEL_REPEATED, ".pulseview.CoreStat")

    _pool.Add(fdp)


_build_pool()


def get_message_class(msg_type: str = MSG_TYPE):
    """返回指定 protobuf 消息类型的动态消息类。"""
    descriptor = _pool.FindMessageTypeByName(msg_type)
    return message_factory.GetMessageClass(descriptor)


def encode_delimited(msg) -> bytes:
    """将单条消息编码为「varint 长度前缀 + 消息体」（length-delimited 流格式）。"""
    data = msg.SerializeToString()
    return encoder._VarintBytes(len(data)) + data


def iter_delimited(buf: bytes, message_class) -> Iterator:
    """从 length-delimited 字节流中逐条解析消息。"""
    pos = 0
    n = len(buf)
    while pos < n:
        size, pos = decoder._DecodeVarint(buf, pos)
        chunk = buf[pos : pos + size]
        pos += size
        msg = message_class()
        msg.ParseFromString(chunk)
        yield msg
