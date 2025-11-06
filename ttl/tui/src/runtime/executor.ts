import type { DataFrame } from '@/types/data.js';

export interface Executor {
	readonly schema: string;
	query(sql: string): Promise<DataFrame>;
	close?(): Promise<void>;
}
