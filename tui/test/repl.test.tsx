import { describe, it, expect, beforeEach } from 'vitest';
import { Repl } from '@/runtime/repl.js';
import { CatalogExecutor } from '@/runtime/catalog_executor.js';

const createTestRuntime = () => {
	const catalog = new CatalogExecutor();
	const executors = new Map();
	executors.set('catalog', catalog);
	return { executors, catalog };
};

describe('Repl', () => {
	let repl: Repl;
	let catalog: CatalogExecutor;

	beforeEach(() => {
		const setup = createTestRuntime();
		repl = new Repl({ executors: setup.executors });
		catalog = setup.catalog;
	});

	it('should start with root pane focused', async () => {
		const focused = await catalog.query("SELECT value FROM catalog.runtime WHERE key = 'focused_pane'");
		expect(focused.rows[0].value).toBe('root');
	});

	it('should create horizontal split with /sh', async () => {
		await repl.execute('/sh');

		const panes = await catalog.query('SELECT * FROM catalog.pane ORDER BY id');
		expect(panes.rows.length).toBe(2);

		const newPane = panes.rows.find((p: any) => p.id.startsWith('pane_'));
		expect(newPane).toBeDefined();
		expect(newPane!.type).toBe('leaf');
		expect(newPane!.direction).toBe('horizontal');
		expect(newPane!.parent_id).toBe('root');
	});

	it('should create vertical split with /sv', async () => {
		await repl.execute('/sv');

		const panes = await catalog.query('SELECT * FROM catalog.pane ORDER BY id');
		expect(panes.rows.length).toBe(2);

		const newPane = panes.rows.find((p: any) => p.id.startsWith('pane_'));
		expect(newPane).toBeDefined();
		expect(newPane!.type).toBe('leaf');
		expect(newPane!.direction).toBe('vertical');
		expect(newPane!.parent_id).toBe('root');
	});

	it('should update focus to new pane after split', async () => {
		await repl.execute('/sh');

		const focused = await catalog.query("SELECT value FROM catalog.runtime WHERE key = 'focused_pane'");
		expect(focused.rows[0].value).toMatch(/^pane_/);
	});

	it('should create nested splits', async () => {
		await repl.execute('/sh');
		await repl.execute('/sv');

		const panes = await catalog.query('SELECT * FROM catalog.pane');
		expect(panes.rows.length).toBe(3);
	});

	it('should handle INSERT queries', async () => {
		await repl.execute("INSERT INTO catalog://runtime (key, value) VALUES ('test_key', 'test_value')");

		const result = await catalog.query("SELECT value FROM catalog.runtime WHERE key = 'test_key'");
		expect(result.rows[0].value).toBe('test_value');
	});

	it('should handle UPDATE queries', async () => {
		await repl.execute("UPDATE catalog://runtime SET value = 'new_root' WHERE key = 'focused_pane'");

		const result = await catalog.query("SELECT value FROM catalog.runtime WHERE key = 'focused_pane'");
		expect(result.rows[0].value).toBe('new_root');
	});

	it('should handle multiple sequential commands', async () => {
		await repl.execute('/sh');
		await repl.execute("INSERT INTO catalog://runtime (key, value) VALUES ('cmd1', 'val1')");
		await repl.execute('/sv');

		const panes = await catalog.query('SELECT * FROM catalog.pane');
		expect(panes.rows.length).toBe(3);

		const runtime_data = await catalog.query("SELECT value FROM catalog.runtime WHERE key = 'cmd1'");
		expect(runtime_data.rows[0].value).toBe('val1');
	});
});
