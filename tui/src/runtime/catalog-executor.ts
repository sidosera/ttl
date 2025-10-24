import type { Executor } from '@/runtime/executor.js';
import type { DataFrame } from '@/types/data.js';
import duckdb from 'duckdb';

export class CatalogExecutor implements Executor {
	readonly schema = 'catalog';
	private db: duckdb.Database;
	private conn: duckdb.Connection;

	constructor() {
		this.db = new duckdb.Database(':memory:');
		this.conn = this.db.connect();
		this.initSchema();
	}

	private initSchema(): void {
		this.conn.exec(`
			CREATE TABLE catalog.pane (
				id TEXT PRIMARY KEY,
				parent_id TEXT,
				type TEXT NOT NULL,
				direction TEXT,
				size DOUBLE,
				widget_type TEXT,
				widget_config TEXT
			)
		`);

		this.conn.exec(`
			CREATE TABLE catalog.runtime (
				key TEXT PRIMARY KEY,
				value TEXT NOT NULL
			)
		`);

		this.conn.exec(`
			INSERT INTO catalog.pane (id, parent_id, type)
			VALUES ('root', NULL, 'container')
		`);

		this.conn.exec(`
			INSERT INTO catalog.runtime (key, value)
			VALUES ('focused_pane', 'root')
		`);
	}

	async query(sql: string): Promise<DataFrame> {
		const result = await new Promise<DataFrame>((resolve, reject) => {
			this.conn.all(sql, (err, rows) => {
				if (err) {
					reject(err);
					return;
				}
				resolve({
					rows: rows || [],
					columns: rows && rows.length > 0 ? Object.keys(rows[0]) : [],
				});
			});
		});

		return result;
	}

	async close(): Promise<void> {
		await new Promise<void>((resolve) => {
			this.conn.close(() => {
				this.db.close(() => {
					resolve();
				});
			});
		});
	}
}
