import type { Executor } from '@/runtime/executor.js';
import type { DataFrame } from '@/types/data.js';

export type Runtime = {
	readonly executors: ReadonlyMap<string, Executor>;
};

export type CommandResult = {
	query: string;
	result: DataFrame;
};

export class Repl {
	constructor(private runtime: Runtime) {}

	stripSchema(query: string): { schema: string; sql: string } | null {
		const schemeMatch = query.match(/(?:FROM|INTO|UPDATE)\s+(\w+):\/\//i);
		if (!schemeMatch) {
			return null;
		}

		const schema = schemeMatch[1];
		const cleanSql = query.replace(new RegExp(`${schema}://`, 'g'), `${schema}.`);

		return { schema, sql: cleanSql };
	}

	async expandMacro(command: string): Promise<string | null> {
		const catalog = this.runtime.executors.get('catalog');
		if (!catalog) {
			return null;
		}

		const parts = command.slice(1).split(/\s+/);
		const macroName = parts[0];
		const args = parts.slice(1);

		const result = await catalog.query(`SELECT query FROM catalog.macro WHERE name = '${macroName}'`);
		if (result.rows.length === 0) {
			return null;
		}

		let query = result.rows[0].query as string;
		args.forEach((arg, idx) => {
			query = query.replace(new RegExp(`\\$${idx + 1}`, 'g'), arg);
		});

		return query;
	}

	async execute(command: string): Promise<CommandResult | null> {
		let query = command.trim();

		if (!query) {
			return null;
		}

		if (query.startsWith('/')) {
			const expanded = await this.expandMacro(query);
			if (!expanded) {
				return null;
			}
			query = expanded;
		}

		const stripped = this.stripSchema(query);
		if (!stripped) {
			return null;
		}

		const executor = this.runtime.executors.get(stripped.schema);
		if (!executor) {
			return null;
		}

		const result = await executor.query(stripped.sql);
		return { query, result };
	}

	formatResult(commandResult: CommandResult): string {
		const { query, result } = commandResult;
		return `Query: ${query}\n\n${result.rows.length} rows\n${JSON.stringify(
			result.rows.slice(0, 5),
			(_, value) => (typeof value === 'bigint' ? value.toString() : value),
			2
		)}`;
	}
}
