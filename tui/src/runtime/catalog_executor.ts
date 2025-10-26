import type { Executor } from '@/runtime/executor.js';
import type { DataFrame } from '@/types/data.js';
import duckdb from 'duckdb';

export class CatalogExecutor implements Executor {
	readonly schema = 'catalog';
	private db: duckdb.Database;
	private conn: duckdb.Connection;
	private listeners: Set<() => void> = new Set();

	constructor() {
		this.db = new duckdb.Database(':memory:');
		this.conn = this.db.connect();
		this.initSchema();
	}

	onDataChanged(callback: () => void): () => void {
		this.listeners.add(callback);
		return () => {
			this.listeners.delete(callback);
		};
	}

	private async computeHash(): Promise<bigint> {
		const tablesResult = await new Promise<DataFrame>((resolve, reject) => {
			this.conn.all("SELECT table_name FROM information_schema.tables WHERE table_schema = 'catalog' ORDER BY table_name", (err, rows) => {
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

		if (tablesResult.rows.length === 0) {
			return BigInt(0);
		}

		const subqueries = tablesResult.rows.map((row: any, idx: number) => {
			const tableName = row.table_name;
			return `
				SELECT ${idx + 1} as tbl_idx, hash(string_agg(row_hash::VARCHAR, ',')) as h
				FROM (SELECT hash('${tableName}', COLUMNS(*)) as row_hash FROM catalog.${tableName})
			`;
		}).join(' UNION ALL ');

		const hashResult = await new Promise<DataFrame>((resolve, reject) => {
			this.conn.all(`
				SELECT SUM(h * power(31, tbl_idx)) % 1000000007 as poly_hash
				FROM (${subqueries})
			`, (err, rows) => {
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

		const hashValue = hashResult.rows[0].poly_hash;
		return hashValue != null ? BigInt(hashValue) : BigInt(0);
	}

	private initSchema(): void {
		this.conn.exec('CREATE SCHEMA IF NOT EXISTS catalog');

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
			CREATE SEQUENCE catalog.history_seq START 1
		`);

		this.conn.exec(`
			CREATE TABLE catalog.history (
				id INTEGER PRIMARY KEY DEFAULT nextval('catalog.history_seq'),
				command TEXT NOT NULL,
				timestamp BIGINT NOT NULL
			)
		`);

		this.conn.exec(`
			CREATE TABLE catalog.macro (
				name TEXT PRIMARY KEY,
				query TEXT NOT NULL
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

		this.conn.exec(`
			INSERT INTO catalog.runtime (key, value) VALUES ('last_pane_id', '')
		`);

		this.conn.exec(`
			INSERT INTO catalog.macro (name, query) VALUES
			('sv', 'INSERT INTO catalog://pane (id, parent_id, type, direction) VALUES (''pane_1_'' || CAST(epoch_ms(now()) AS VARCHAR), (SELECT value FROM catalog://runtime WHERE key = ''focused_pane''), ''leaf'', ''vertical''), (''pane_2_'' || CAST(epoch_ms(now()) AS VARCHAR), (SELECT value FROM catalog://runtime WHERE key = ''focused_pane''), ''leaf'', ''vertical''); UPDATE catalog://runtime SET value = (SELECT id FROM catalog://pane WHERE id LIKE ''pane_2_%'' ORDER BY id DESC LIMIT 1) WHERE key = ''focused_pane'''),
			('sh', 'INSERT INTO catalog://pane (id, parent_id, type, direction) VALUES (''pane_1_'' || CAST(epoch_ms(now()) AS VARCHAR), (SELECT value FROM catalog://runtime WHERE key = ''focused_pane''), ''leaf'', ''horizontal''), (''pane_2_'' || CAST(epoch_ms(now()) AS VARCHAR), (SELECT value FROM catalog://runtime WHERE key = ''focused_pane''), ''leaf'', ''horizontal''); UPDATE catalog://runtime SET value = (SELECT id FROM catalog://pane WHERE id LIKE ''pane_2_%'' ORDER BY id DESC LIMIT 1) WHERE key = ''focused_pane''')
		`);
	}

	async query(sql: string): Promise<DataFrame> {
		const hashBefore = await this.computeHash();

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

		const hashAfter = await this.computeHash();

		if (hashBefore !== hashAfter) {
			this.listeners.forEach(listener => listener());
		}

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
