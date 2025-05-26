"use client";

import { LogViewer } from '@/components/LogViewer';
import { Button } from '@/components/ui/button';
import { ArrowLeft } from 'lucide-react';
import Link from 'next/link';

export default function LogsPage() {
  return (
    <main className="min-h-screen bg-background p-8">
      <div className="max-w-7xl mx-auto space-y-8">
        <div className="flex items-center gap-4">
          <Link href="/">
            <Button variant="outline" size="sm">
              <ArrowLeft className="mr-2 h-4 w-4" />
              Voltar
            </Button>
          </Link>
          <div>
            <h1 className="text-4xl font-bold text-foreground">Logs do Sistema</h1>
            <p className="text-muted-foreground mt-2">
              Visualize os logs de operação do sistema
            </p>
          </div>
        </div>
        <LogViewer />
      </div>
    </main>
  );
} 