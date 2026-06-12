interface ApiResponse<T> {
  dat: T;
  err: string;
}

async function request<T>(url: string, options?: RequestInit): Promise<T> {
  const res = await fetch(url, {
    headers: { 'Content-Type': 'application/json' },
    ...options,
  });
  const json: ApiResponse<T> = await res.json();
  if (!res.ok || json.err) {
    throw new Error(json.err || `Request failed: ${res.status}`);
  }
  return json.dat;
}

export default request;
