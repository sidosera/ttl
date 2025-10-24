import { useState } from 'react';
import { useRuntime } from '@/context/RuntimeContext.js';

export function useHistory() {
	const runtime = useRuntime();
	const [historyIndex, setHistoryIndex] = useState(-1);

	const addToHistory = async (command: string) => {
		const catalog = runtime.executors.get('catalog');
		if (!catalog) return;

		const escaped = command.replace(/'/g, "''");
		await catalog.query(`INSERT INTO catalog.history (command, timestamp) VALUES ('${escaped}', ${Date.now()})`);
		setHistoryIndex(-1);
	};

	const navigateHistory = async (direction: 'up' | 'down') => {
		const catalog = runtime.executors.get('catalog');
		if (!catalog) return null;

		const result = await catalog.query('SELECT command FROM catalog.history ORDER BY id DESC');
		const history = result.rows.map((r: any) => r.command);

		if (history.length === 0) return null;

		let newIndex = historyIndex;
		if (direction === 'up') {
			newIndex = Math.min(historyIndex + 1, history.length - 1);
		} else {
			newIndex = Math.max(-1, historyIndex - 1);
		}

		setHistoryIndex(newIndex);
		return newIndex >= 0 ? history[newIndex] : '';
	};

	return { addToHistory, navigateHistory };
}
