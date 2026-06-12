import { Input } from 'antd';
import './style.less';

export interface PromQLInputProps {
  value?: string;
  placeholder?: string;
  readOnly?: boolean;
  onChange?: (value?: string) => void;
  onEnter?: (value?: string) => void;
}

export default function PromQLInput({
  value,
  placeholder = '输入 PromQL 查询，Ctrl+Enter 执行',
  readOnly,
  onChange,
  onEnter,
}: PromQLInputProps) {
  return (
    <Input.TextArea
      className='promql-input'
      value={value}
      readOnly={readOnly}
      placeholder={placeholder}
      autoSize={{ minRows: 2, maxRows: 6 }}
      onChange={(e) => onChange?.(e.target.value)}
      onKeyDown={(e) => {
        if (e.key === 'Enter' && (e.ctrlKey || e.metaKey)) {
          e.preventDefault();
          onEnter?.((e.target as HTMLTextAreaElement).value);
        }
      }}
    />
  );
}
