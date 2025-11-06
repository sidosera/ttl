import { useState, useEffect } from 'react';
import { useRuntime } from '@/context/runtime_context.js';
import type { CatalogExecutor } from '@/runtime/catalog_executor.js';

export type PaneData = {
	id: string;
	parent_id: string | null;
	type: 'leaf' | 'split';
	direction: 'horizontal' | 'vertical' | null;
	size: number | null;
	widget_type: string | null;
	widget_config: string | null;
};

export function usePanes() {
	const runtime = useRuntime();
	const catalog = runtime.executors.get('catalog') as CatalogExecutor;
	const [panes, setPanes] = useState<PaneData[]>([]);
	const [focusedPaneId, setFocusedPaneId] = useState<string>('root');

	useEffect(() => {
		if (!catalog) return;

		const load = async () => {
			const [panesResult, focusedResult] = await Promise.all([
				catalog.query('SELECT * FROM catalog.pane'),
				catalog.query("SELECT value FROM catalog.runtime WHERE key = 'focused_pane'"),
			]);

			setPanes(panesResult.rows as PaneData[]);
			setFocusedPaneId(
				focusedResult?.rows[0]?.value?.toString() ?? 'root'
			);
		};
		
		load();

		return catalog.onDataChanged(() => {
			load();
		});
	}, [catalog]);

	return { panes, focusedPaneId };
}
